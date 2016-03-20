#pragma comment(lib, "ws2_32.lib")
// note, would use RIO if did not have to be compatible with Windows 7, but current approach is performant enough
// the code handling IOCP in this implementation is based on the server presented here:
// http://www.serverframework.com/asynchronousevents/2012/08/winsock-registered-io---traditional-multi-threaded-iocp-udp-example-server.html

#include "UDPMultiCast.h"

#include "utils.h"
#include "git_refid.h"

#include <iostream>
#include <process.h>
#include <cstring>

UDPMultiCast::UDPMultiCast()
{
}

UDPMultiCast::~UDPMultiCast()
{
    deInit();
}

void UDPMultiCast::init()
{
    // initialize timestamp provider
    timeUtils::initTimeStamping(_setMaxClockRes,_useWTP);

    // InitialiseWinsock
    WSADATA data;
    if (0 != ::WSAStartup(MAKEWORD(2, 2), &data))
    {
        ErrorExit("WSAStartup");
    }

    // create and set up socket
    _socket = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (_socket == INVALID_SOCKET)
    {
        ErrorExit("WSASocket");
    }

    // setup socket
    // TODO: read and understand: http://stackoverflow.com/questions/10692956/what-does-it-mean-to-bind-a-multicast-udp-socket
    // also, for a client it is recommended by the MSDN docs (without given reason) to not bind but WSAJoinLeaf
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;	// INADDR_ANY or use inet_addr("127.0.0.1") to set
    if (SOCKET_ERROR == ::bind(_socket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)))
    {
        ErrorExit("bind");
    }

    // setup multicasting: joining group
    setupAndJoinMultiCast();

    // setup multicasting: set loopback
    setupLoopBack(_loopBack);

    // multicast note: can use IP_BLOCK_SOURCE and IP_UNBLOCK_SOURCE to control what messages we receive.
    // Though separate groups is a better idea if we want multiple smaller groups

    // setup IOCP port and associate socket handle
    _hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (0 == _hIOCP)
    {
        ErrorExit("CreateIoCompletionPort");
    }
    if (0 == ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(_socket), _hIOCP, 1, 0))
    {
        ErrorExit("CreateIoCompletionPort");
    }

    // create receive buffer and post a bunch of receives
    DWORD receiveBuffersAllocated;
    _pReceiveBuffers = reinterpret_cast<char *>(AllocateBufferSpace(_bufferSize, _IOCPPendingReceives, receiveBuffersAllocated));
    _pAddrBuffers    = reinterpret_cast<sockaddr_in *>(AllocateBufferSpace(sizeof(sockaddr_in), _IOCPPendingReceives, receiveBuffersAllocated));
    _pExtOverlappedArray = new EXTENDED_OVERLAPPED[receiveBuffersAllocated];

    for (DWORD i = 0, offset = 0; i < receiveBuffersAllocated; ++i, offset += static_cast<DWORD>(_bufferSize))
    {
        EXTENDED_OVERLAPPED *pExtOverlapped = _pExtOverlappedArray + i;

        ZeroMemory(pExtOverlapped, sizeof(EXTENDED_OVERLAPPED));

        pExtOverlapped->buf.buf = _pReceiveBuffers + offset;
        pExtOverlapped->buf.len = static_cast<ULONG>(_bufferSize);	// lets assume the buffer size does not exceed 32bit limits
        pExtOverlapped->isRead = TRUE;
        pExtOverlapped->addr = _pAddrBuffers + i;
        pExtOverlapped->addrLen = sizeof(sockaddr_in);

        // queue up receive request
        receive(pExtOverlapped);
    }
    std::cout << _IOCPPendingReceives << " receives pending" << std::endl;

    // Create IOCP processing threads
    ::InitializeCriticalSectionAndSpinCount(&_criticalSection, _spinCount);
    _haveCriticalSection = true;

    // Start our worker threads
    for (DWORD i = 0; i < _numIOCPThreads; ++i)
    {
        unsigned int notUsed;

        const uintptr_t result = ::_beginthreadex(0, 0, startThreadFunction, (void*)this, 0, &notUsed);

        if (result == 0)
        {
            ErrorExit("_beginthreadex", errno);
        }

        _threads.push_back(reinterpret_cast<HANDLE>(result));
    }

    std::cout << _numIOCPThreads << " threads running" << std::endl;

    // done
    _initialized = true;
}

void UDPMultiCast::deInit()
{
    if (!_initialized)
        // nothing to do
        return;

    // leave multicast group
    leaveMultiCast();

    // send exit packet to IOCP threads and wait till all exited
    stopIOCP();
    waitIOCPThreadsStop();

    // release socket
    if (SOCKET_ERROR == ::shutdown(_socket,2))
    {
        ErrorExit("shutdown");
    }
    if (SOCKET_ERROR == ::closesocket(_socket))
    {
        ErrorExit("closesocket");
    }
    // cleanup WinSocks
    WSACleanup();

    // free memory
    delete[] _pExtOverlappedArray;
    VirtualFree(_pReceiveBuffers, 0, MEM_RELEASE);

    _initialized = false;
}

void UDPMultiCast::sendWithTimeStamp(const std::string msg_, const char delim_/* = ','*/)
{
    send(msg_ + delim_ + std::to_string(timeUtils::getTimeStamp()));
}

void UDPMultiCast::send(const std::string msg_)
{
    EXTENDED_OVERLAPPED* sendOverlapped = new EXTENDED_OVERLAPPED();
    sendOverlapped->buf.buf = new char[_bufferSize]();

    // if not resending whats in buffer, clear buffer and copy in new message
    if (!msg_.empty())
    {
        // copy in new msg, truncating to buffersize if needed
        size_t msgLen = std::min(msg_.size(), _bufferSize);
        strncpy_s(sendOverlapped->buf.buf, _bufferSize, msg_.c_str(), msgLen);
        // indicate how much data we actually have. Note that allocated buffer remains of size bufferSize
        sendOverlapped->buf.len = static_cast<ULONG>(msgLen*sizeof(std::string::value_type));	// lets assume the buffer size (msg is truncated to that) does not exceed 32bit limits
    }
    //cout << "sending:  \"" << _sendOverlapped.buf.buf << "\"" << endl;

    sendInternal(sendOverlapped);
}

void UDPMultiCast::sendInternal(EXTENDED_OVERLAPPED * sendOverlapped_)
{
    // do send
    DWORD bytesSent = 0;
    if (SOCKET_ERROR == ::WSASendTo(_socket, &(sendOverlapped_->buf), 1, &bytesSent, 0, reinterpret_cast<sockaddr*>(&_multiCastGroup), sizeof(_multiCastGroup), static_cast<OVERLAPPED *>(sendOverlapped_), 0))
    {
        const DWORD lastError = ::GetLastError();

        if (lastError != ERROR_IO_PENDING)
        {
            ErrorExit("WSASendTo", __LINE__, lastError);
        }
    }
}

void UDPMultiCast::receive(EXTENDED_OVERLAPPED* pExtOverlapped_, OVERLAPPED* pOverlapped/* = nullptr*/)
{
    DWORD bytesRecvd = 0;
    DWORD flags = 0;

    if (!pOverlapped)
    {
        pOverlapped = static_cast<OVERLAPPED *>(pExtOverlapped_);
    }
    if (SOCKET_ERROR == ::WSARecvFrom(_socket, &(pExtOverlapped_->buf), 1, &bytesRecvd, &flags, (sockaddr*) pExtOverlapped_->addr, &(pExtOverlapped_->addrLen), pOverlapped, 0))
    {
        const DWORD lastError = ::GetLastError();

        if (lastError != ERROR_IO_PENDING)
        {
            ErrorExit("WSARecvFrom", __LINE__, lastError);
        }
    }
}

std::string UDPMultiCast::getGitRefID() const
{
    return GIT_REFID;
}

void UDPMultiCast::setUseWTP(bool useWTP_)
{
    if (_initialized)
        ErrorMsgExit("cannot set usage of WTP timestamping when already initialized");

    _useWTP = useWTP_;
}

void UDPMultiCast::setMaxClockRes(bool setMaxClockRes_)
{
    if (_initialized)
        ErrorMsgExit("cannot set maximum clock resolution when already initialized");

    _setMaxClockRes = setMaxClockRes_;
}

void UDPMultiCast::setLoopBack(const BOOL& loopBack_)
{
    if (_initialized)
    {
        setupLoopBack(loopBack_);
    }
    _loopBack = loopBack_;
}

void UDPMultiCast::setGroupAddress(const std::string& groupAddress_)
{
    // leave old group, if any
    leaveMultiCast();
    // set new group address and join
    _groupAddress = groupAddress_;
    setupAndJoinMultiCast();
    // setup loopback for new group
    setupLoopBack(_loopBack);
}

void UDPMultiCast::setPort(const unsigned short& port_)
{
    if (_initialized)
        ErrorMsgExit("cannot set port when already initialized");

    _port = port_;
}

void UDPMultiCast::setBufferSize(const size_t & bufferSize_)
{
    if (_initialized)
        ErrorMsgExit("cannot set buffer size when already initialized");

    _bufferSize = bufferSize_;
}

void UDPMultiCast::setNumQueuedReceives(const unsigned long & IOCPPendingReceives_)
{
    if (_initialized)
        ErrorMsgExit("cannot set number of queued receives when already initialized");

    _IOCPPendingReceives = IOCPPendingReceives_;
}

void UDPMultiCast::setNumReceiverThreads(const unsigned long & numIOCPThreads_)
{
    if (_initialized)
        ErrorMsgExit("cannot set number of receiver threads when already initialized");

    _numIOCPThreads = numIOCPThreads_;
}

template<size_t buffer_size>
inline std::vector<message> getMsgs(queue_t<buffer_size>& msgBuffer_)
{
    std::vector<message> dataMsgs;
    while (true)
    {
        message temp;
        bool success = msgBuffer_.dequeue(temp);
        if (success)
            dataMsgs.push_back(std::move(temp));
        else
            break;
    }
    return dataMsgs;
}

std::vector<message> UDPMultiCast::getData()
{
    return getMsgs(_receivedData);
}

std::vector<message> UDPMultiCast::getCommands()
{
    return getMsgs(_receivedCommands);
}

void UDPMultiCast::stopIOCP()
{
    // post exit message
    PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
}

void UDPMultiCast::waitIOCPThreadsStop()
{
    // Wait for all threads to exit

    if (_threads.empty() && !_haveCriticalSection)
        return;

    for (auto hThread : _threads)
    {
        if (WAIT_OBJECT_0 != ::WaitForSingleObject(hThread, INFINITE))
        {
            ErrorExit("WaitForSingleObject");
        }

        ::CloseHandle(hThread);
    }
    _threads.clear();

    ::DeleteCriticalSection(&_criticalSection);
    _haveCriticalSection = false;
}

int UDPMultiCast::checkReceiverThreads()
{
    // Check if threads still active
    auto it = _threads.begin();
    while (it != _threads.end())
    {
        if (WAIT_OBJECT_0 == ::WaitForSingleObject(*it, 0))
        {
            ::CloseHandle(*it);
            it = _threads.erase(it);
        }
        else
            ++it;
    }

    if (_threads.empty() && _haveCriticalSection)
    {
        std::cout << "all threads dead" << std::endl;
        ::DeleteCriticalSection(&_criticalSection);
        _haveCriticalSection = false;
    }

    return static_cast<int>(_threads.size());
}

unsigned int UDPMultiCast::startThreadFunction(void *pV)
{
    UDPMultiCast* p = static_cast<UDPMultiCast*>(pV);
    p->threadFunction();
    return 0;
}

unsigned int UDPMultiCast::threadFunction()
{
    DWORD numberOfBytes = 0;
    ULONG_PTR completionKey = 0;
    OVERLAPPED *pOverlapped = 0;

    for (;;)
    {
        if (!::GetQueuedCompletionStatus(_hIOCP, &numberOfBytes, &completionKey, &pOverlapped, INFINITE))
        {
            if (!pOverlapped)
            {
                // failed to dequeue a completion packet.
                // I read this can be because of time out, but not for us as we set INFINITE
                // don't know what other reason there could be, error message may reveal something...
                ErrorExit("GetQueuedCompletionStatus");
            }
            else
            {
                // pOverlapped not NULL, this means an IO error occurred
                EXTENDED_OVERLAPPED *pExtOverlapped = static_cast<EXTENDED_OVERLAPPED *>(pOverlapped);

                if (pExtOverlapped->isRead)
                {
                    // too bad... just queue up a new read
                    receive(pExtOverlapped,pOverlapped);
                }
                else
                {
                    // attempt resend (NB: don't update timestamp, this is processing latency here...)
                    sendInternal(pExtOverlapped);
                }
            }
            continue;
        }

        if (numberOfBytes == 0 && completionKey == 0 && pOverlapped == 0)
        {
            // zero size and zero handle is my termination message
            // re-post, then break, so all threads on the IOCP will
            // one by one wake up and exit in a controlled manner
            PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
            break;
        }
        else
        {
            // received or sent data
            EXTENDED_OVERLAPPED *pExtOverlapped = static_cast<EXTENDED_OVERLAPPED *>(pOverlapped);

            if (pExtOverlapped->isRead)
            {
                message received;
                received.timeStamp = timeUtils::getTimeStamp();
                inet_ntop(AF_INET, &(reinterpret_cast<sockaddr_in *>(pExtOverlapped->addr)->sin_addr), received.ip, INET_ADDRSTRLEN);
                // parse message, store it, and see what to do with it
                size_t headerLen, msgLen;
                auto action = processMsg(pExtOverlapped->buf.buf, &headerLen, &msgLen);
                received.text = msgLen >= headerLen ? _strdup(pExtOverlapped->buf.buf + headerLen) : new char{ '\0' }; // make sure always valid (if possibly empty) string
                //cout << "read data: \"" << pExtOverlapped->buf.buf << "\" from " << addr << endl;

                // adds to output queue or takes indicated action 
                switch (action)
                {
                case MsgAction::noAction:
                    // nothing to do
                    break;
                case MsgAction::exit:
                    // exit msg received, post exit msg to other threads and exit
                    PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
                    break;
                case MsgAction::storeData:
                    _receivedData.enqueue(received);
                    break;
                case MsgAction::storeCommand:
                    _receivedCommands.enqueue(received);
                    break;
                }

                // queue up new receive request. Note that a completion packet will be queued up even if this returns immediately
                receive(pExtOverlapped, pOverlapped);
            }
            else
            {
                // send completed
                //cout << "sent data: \"" << pExtOverlapped->buf.buf << "\"" << endl;
                //Send();	// just keep sending same message for now

                delete[] pExtOverlapped->buf.buf;
                // addr field not used for sends, nothing to delete
                delete pExtOverlapped;
            }
        }
    }

    return 0;
}

MsgAction UDPMultiCast::processMsg(const char * msg_, size_t *len_, size_t *msgLen_)
{
    auto commaPos = strchr(msg_, ',');
    *msgLen_ = strlen(msg_);
    if (!commaPos)
        *len_ = *msgLen_ + 1; // pointing one past msg end to match below
    else
        *len_ = commaPos - msg_ + 1;
    char *id = new char[*len_] {0};	// as includes comma, we've got a null terminator
    memcpy(id, msg_, *len_ - 1);	// -1 is safe as we're pointing to idx 1 at minimum

    // switchyard using consexpr str2int to hash switch values to integrals
    switch (str2int(id))
    {
    case str2int("exit"):
        return MsgAction::exit;
    case str2int("dat"):
        return MsgAction::storeData;
    case str2int("cmd"):
        return MsgAction::storeCommand;
    default:
        return MsgAction::noAction;
    }
}

void UDPMultiCast::setupLoopBack(const BOOL loopBack_)
{
    if (SOCKET_ERROR == ::setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast<const char *>(&loopBack_), sizeof(loopBack_)))
    {
        ErrorExit("setsockopt IP_MULTICAST_LOOP");
    }
}

void UDPMultiCast::setupAndJoinMultiCast()
{
    _multiCastGroup.sin_family = AF_INET;
    _multiCastGroup.sin_port = htons(_port);
    inet_pton(AF_INET, _groupAddress.c_str(), &(_multiCastGroup.sin_addr));
    if (_multiCastGroup.sin_addr.s_addr == INADDR_NONE)
    {
        ErrorExit("The target ip address entered must be a legal IPv4 address");
    }

    _multiCastRequest.imr_multiaddr.s_addr = _multiCastGroup.sin_addr.S_un.S_addr;
    _multiCastRequest.imr_interface.s_addr = INADDR_ANY;
    if (SOCKET_ERROR == ::setsockopt(_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char *>(&_multiCastRequest), sizeof(_multiCastRequest)))
    {
        ErrorExit("setsockopt IP_ADD_MEMBERSHIP");
    }

    _multiCastJoined = true;
}

void UDPMultiCast::leaveMultiCast()
{
    if (_multiCastJoined)
    {
        if (SOCKET_ERROR == ::setsockopt(_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, reinterpret_cast<char *>(&_multiCastRequest), sizeof(_multiCastRequest)))
        {
            ErrorExit("setsockopt IP_DROP_MEMBERSHIP");
        }
    }
    _multiCastJoined = false;
}
