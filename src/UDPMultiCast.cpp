#pragma comment(lib, "ws2_32.lib")
// note, would use RIO if did not have to be compatible with Windows 7, but current approach is performant enough
// the code handling IOCP in this implementation is based on the server presented here:
// http://www.serverframework.com/asynchronousevents/2012/08/winsock-registered-io---traditional-multi-threaded-iocp-udp-example-server.html

#include "UDPMultiCast/UDPMultiCast.h"

#include "UDPMultiCast/utils.h"
#include "git_refid.h"


#ifdef HAS_SMI_INTEGRATION
#   include "iViewXAPI.h"
#   if _WIN64
#	    pragma comment(lib, "iViewXAPI64.lib")
#   else
#	    pragma comment(lib, "iViewXAPI.lib")
#   endif
#endif
#ifdef HAS_TOBII_INTEGRATION
#   include <tobii_research.h>
#   include <tobii_research_eyetracker.h>
#   include <tobii_research_streams.h>
#   pragma comment(lib, "tobii_research.lib")
#endif

#include <iostream>
#include <process.h>
#include <cstring>
#include <cmath>

UDPMultiCast::UDPMultiCast()
{
    // initialize timestamp provider. When user changes settings, timestamper is automatically inited again
    timeUtils::initTimeStamping(_setMaxClockRes,_useWTP);
}

UDPMultiCast::~UDPMultiCast()
{
    deInit();
}

void UDPMultiCast::init()
{
    // InitialiseWinsock
    WSADATA data;
    if (0 != ::WSAStartup(MAKEWORD(2, 2), &data))
    {
        ErrorExit("WSAStartup");
    }

    // create and set up socket
    _socket.set(::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED));
    if (_socket.get() == INVALID_SOCKET)
    {
        ErrorExit("WSASocket");
    }

    // setup socket
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;	// INADDR_ANY or use inet_addr("127.0.0.1") to set

    // if wanted, set the socket up for reuse of the port, so multiple applications can access data incoming over the same port
    // with winsocks, for a multicast socket, all sockets opened on the same port with SO_REUSEADDR receive all incoming packets
    if (_reuseSocket)
    {
        if (SOCKET_ERROR == ::setsockopt(_socket.get(), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&_reuseSocket), sizeof(_reuseSocket)))
        {
            ErrorExit("setsockopt SO_REUSEADDR");
        }
    }

    // TODO: read and understand: http://stackoverflow.com/questions/10692956/what-does-it-mean-to-bind-a-multicast-udp-socket
    // also, for a client it is recommended by the MSDN docs (without given reason) to not bind but WSAJoinLeaf
    if (SOCKET_ERROR == ::bind(_socket.get(), reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)))
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
    _hIOCP.set(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0));
    if (0 == _hIOCP.get())
    {
        ErrorExit("CreateIoCompletionPort");
    }
    if (0 == ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(_socket.get()), _hIOCP.get(), 1, 0))
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

        RtlSecureZeroMemory(pExtOverlapped, sizeof(EXTENDED_OVERLAPPED));

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

    // remove smi callback
#ifdef HAS_SMI_INTEGRATION
    if (_smiDataSenderStarted)
        removeSMIDataSender();
#endif

    // remove tobii callback
#ifdef HAS_TOBII_INTEGRATION
    if (_tobiiDataSenderStarted)
        removeTobiiDataSender();
#endif

    // leave multicast group
    leaveMultiCast();

    // send exit packet to IOCP threads and wait till all exited
    stopIOCP();
    waitIOCPThreadsStop();
    _hIOCP.cleanUp();

    // release socket
    _socket.cleanUp();
    // cleanup WinSocks
    WSACleanup();

    // free memory
    if (_pExtOverlappedArray)
        delete[] _pExtOverlappedArray;
    if (_pReceiveBuffers)
        VirtualFree(_pReceiveBuffers, 0, MEM_RELEASE);
    if (_pAddrBuffers)
        VirtualFree(_pAddrBuffers, 0, MEM_RELEASE);

    _initialized = false;
}

int64_t UDPMultiCast::sendWithTimeStamp(const std::string msg_, const char delim_/* = ','*/)
{
    int64_t timeStamp = timeUtils::getTimeStamp();
    send(msg_ + delim_ + std::to_string(timeStamp));

    return timeStamp;
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
    // sendInternal goes so fast, despite being asynchronous (according to testing) that
    // memory for the message can be deallocated by the time we print to the SMI file below.
    // Ergo: do NOT use sendOverlapped->buf after this point

#ifdef HAS_SMI_INTEGRATION
    // send command directly to file so that we know as precisely (and with as low latency) as possible when it arrived
    // this can be used for sync between pairs of systems
    size_t headerLen_, msgLen_;
    if (MsgType::command == UDPMultiCast::processMsg(msg_.c_str(), &headerLen_, &msgLen_))
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "send: %s", msg_.c_str());
        iV_SendImageMessage(buf);
    }
#endif
    // nothing to log for Tobii, system timestamp added when sending is sufficient
}

void UDPMultiCast::sendInternal(EXTENDED_OVERLAPPED* sendOverlapped_)
{
    // do send
    DWORD bytesSent = 0;
    if (SOCKET_ERROR == ::WSASendTo(_socket.get(), &(sendOverlapped_->buf), 1, &bytesSent, 0, reinterpret_cast<sockaddr*>(&_multiCastGroup), sizeof(_multiCastGroup), static_cast<OVERLAPPED *>(sendOverlapped_), 0))
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
    if (SOCKET_ERROR == ::WSARecvFrom(_socket.get(), &(pExtOverlapped_->buf), 1, &bytesRecvd, &flags, (sockaddr*) pExtOverlapped_->addr, &(pExtOverlapped_->addrLen), pOverlapped, 0))
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
    _useWTP = useWTP_;

    // re-initialize timestamp provider
    timeUtils::initTimeStamping(_setMaxClockRes,_useWTP);
}

void UDPMultiCast::setMaxClockRes(bool setMaxClockRes_)
{
    _setMaxClockRes = setMaxClockRes_;

    // re-initialize timestamp provider
    timeUtils::initTimeStamping(_setMaxClockRes,_useWTP);
}

void UDPMultiCast::setLoopBack(const BOOL& loopBack_)
{
    if (_initialized)
    {
        setupLoopBack(loopBack_);
    }
    _loopBack = loopBack_;
}

void UDPMultiCast::setReuseSocket(const BOOL& value_)
{
    if (_initialized)
        ErrorMsgExit("cannot change whether socket can be reused when already initialized");

    _reuseSocket = value_;
}

void UDPMultiCast::setComputerFilter(double* computerFilter_, size_t numElements_)
{
    if (!numElements_ || computerFilter_ == nullptr)
    {
        _computerFilter.clear();
        _hasComputerFilter = false;
    }
    else
    {
        for (size_t i = 0; i < numElements_; i++)
            _computerFilter.insert(char(computerFilter_[i] + .5));

        _hasComputerFilter = true;
    }
}

void UDPMultiCast::setGroupAddress(const std::string& groupAddress_)
{
    // set new group address and join
    _groupAddress = groupAddress_;

    // if already inited, leave the old group and join the new one
    if (_initialized)
    {
        // leave old group
        leaveMultiCast();
        // join new group
        setupAndJoinMultiCast();
        // setup loopback for new group
        setupLoopBack(_loopBack);
    }
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
    if (_hIOCP.get() != INVALID_HANDLE_VALUE)
        PostQueuedCompletionStatus(_hIOCP.get(), 0, 0, 0);
}

void UDPMultiCast::waitIOCPThreadsStop()
{
    // Wait for all threads to exit

    if (_threads.empty())
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

    if (_threads.empty())
    {
        std::cout << "all threads dead" << std::endl;
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
        if (!::GetQueuedCompletionStatus(_hIOCP.get(), &numberOfBytes, &completionKey, &pOverlapped, INFINITE))
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
            PostQueuedCompletionStatus(_hIOCP.get(), 0, 0, 0);
            break;
        }
        else
        {
            // received or sent data
            EXTENDED_OVERLAPPED *pExtOverlapped = static_cast<EXTENDED_OVERLAPPED *>(pOverlapped);

            if (pExtOverlapped->isRead)
            {
                // get timestamp asap
                int64_t receiveTimeStamp = timeUtils::getTimeStamp();
#ifdef IP_ADDR_AS_STR
                // convert ip address to string.
                // never any need for this, we just need the end of the ip address as number
                inet_ntop(AF_INET, &(reinterpret_cast<sockaddr_in *>(pExtOverlapped->addr)->sin_addr), received.ip, INET_ADDRSTRLEN);
#else
                auto senderIP = pExtOverlapped->addr->sin_addr.S_un.S_un_b.s_b4;
#endif
                // if no filter, process all messages. if filter, process only the specified messages
                if (!_hasComputerFilter || _computerFilter.find(senderIP) != _computerFilter.end())
                {
                    // parse message, store it, and see what to do with it
                    size_t headerLen, msgLen;
                    auto action = processMsg(pExtOverlapped->buf.buf, &headerLen, &msgLen);
                    auto msgStart = pExtOverlapped->buf.buf + headerLen; // always ok. if we end up in bit of memory that is null, strdup in message constructor will simply give us an empty string

                    // adds to output queue or takes indicated action
                    switch (action)
                    {
                    case MsgType::unknown:
                        // nothing to do
                        break;
                    case MsgType::exit:
                        // exit msg received, post exit msg to other threads and exit
                        PostQueuedCompletionStatus(_hIOCP.get(), 0, 0, 0);
                        break;
                    case MsgType::data:
                        _receivedData.enqueue(message(senderIP,msgStart,receiveTimeStamp));
                        break;
                    case MsgType::command:
                        _receivedCommands.enqueue(message(senderIP,msgStart,receiveTimeStamp));
#ifdef HAS_SMI_INTEGRATION
                        // send command directly to file so that we know as precisely (and with as low latency) as possible when it arrived
                        // this can be used for sync between pairs of systems
                        if (msgLen >= headerLen)
                        {
                            char buf[256];
                            snprintf(buf, sizeof(buf), "%d: %s,%lld", senderIP, pExtOverlapped->buf.buf, receiveTimeStamp);
                            iV_SendImageMessage(buf);
                        }
#endif
                        // nothing to log for Tobii: system timestamp set when receiving is sufficient
                        break;
                    }

                    // overwrite msg with null so we keep our receive buffer clean
                    RtlSecureZeroMemory(pExtOverlapped->buf.buf,msgLen);
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

MsgType UDPMultiCast::processMsg(const char *msg_, size_t *headerLen_, size_t *msgLen_)
{
    // does not modify msg_
    // find where comma separating header from msg is
    auto commaPos = strchr(msg_, ',');
    // see how long the whole input is
    *msgLen_ = strlen(msg_);
    // get length of header
    if (!commaPos)
        *headerLen_ = *msgLen_ + 1; // pointing one past msg end to match below
    else
        *headerLen_ = commaPos - msg_ + 1;
    // take copy of header
    char *header = new char[*headerLen_] {0};	// as headerLen_ includes comma, we've got space for null terminator
    memcpy(header, msg_, *headerLen_ - 1);	    // -1 is safe as we're pointing to idx 1 at minimum

    return strToMsgType(header);
}

void UDPMultiCast::setupLoopBack(const BOOL loopBack_)
{
    if (SOCKET_ERROR == ::setsockopt(_socket.get(), IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast<const char *>(&loopBack_), sizeof(loopBack_)))
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
    if (SOCKET_ERROR == ::setsockopt(_socket.get(), IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char *>(&_multiCastRequest), sizeof(_multiCastRequest)))
    {
        ErrorExit("setsockopt IP_ADD_MEMBERSHIP");
    }

    _multiCastJoined = true;
}

void UDPMultiCast::leaveMultiCast()
{
    if (_multiCastJoined)
    {
        if (SOCKET_ERROR == ::setsockopt(_socket.get(), IPPROTO_IP, IP_DROP_MEMBERSHIP, reinterpret_cast<char *>(&_multiCastRequest), sizeof(_multiCastRequest)))
        {
            ErrorExit("setsockopt IP_DROP_MEMBERSHIP");
        }
    }
    _multiCastJoined = false;
}

#ifdef HAS_SMI_INTEGRATION
// callback must be a free function
namespace {
    UDPMultiCast* classPtr = nullptr;
    int __stdcall SMISampleCallback(SampleStruct sampleData)
    {
        char buf[128] = { '\0' };
        sprintf_s(buf, "dat,%lld,%.2f,%.2f,%.2f,%.2f", sampleData.timestamp, sampleData.leftEye.gazeX, sampleData.leftEye.gazeY, sampleData.rightEye.gazeX, sampleData.rightEye.gazeY);
        classPtr->sendWithTimeStamp(buf);
        return 1;
    }
}
void UDPMultiCast::startSMIDataSender()
{
    classPtr = this;
    if (RET_SUCCESS != iV_SetSampleCallback(SMISampleCallback))
        DoExitWithMsg("startSMIDataSender: iV_SetSampleCallback failed");
    _smiDataSenderStarted = true;
}
void UDPMultiCast::removeSMIDataSender()
{
    iV_SetSampleCallback(nullptr);
}

#endif


#ifdef HAS_TOBII_INTEGRATION
// callback must be a free function
void TobiiSampleCallback(TobiiResearchGazeData* gaze_data_, void* user_data)
{
    if (user_data)
    {
        char buf[128] = { '\0' };
        // bunch of crap to make sure we get a simple quiet nan, not the indefinite nan that is formatted as -nan(ind)
        auto scrSize = static_cast<UDPMultiCast*>(user_data)->_scrSize;
        if (scrSize.empty())
            return;

        auto lx = gaze_data_->left_eye .gaze_point.position_on_display_area.x*scrSize[0];
        if (std::isnan(lx))
            lx = std::nanf("");
        auto ly = gaze_data_->left_eye .gaze_point.position_on_display_area.y*scrSize[1];
        if (std::isnan(ly))
            ly = std::nanf("");
        auto rx = gaze_data_->right_eye.gaze_point.position_on_display_area.x*scrSize[0];
        if (std::isnan(rx))
            rx = std::nanf("");
        auto ry = gaze_data_->right_eye.gaze_point.position_on_display_area.y*scrSize[1];
        if (std::isnan(ry))
            ry = std::nanf("");

        sprintf_s(buf, "dat,%lld,%.2f,%.2f,%.2f,%.2f", gaze_data_->system_time_stamp, lx,ly, rx,ry);
        static_cast<UDPMultiCast*>(user_data)->sendWithTimeStamp(buf);
    }
}

bool UDPMultiCast::connectToTobii(std::string address_)
{
    TobiiResearchStatus status = tobii_research_get_eyetracker(address_.c_str(),&_eyeTracker);
    if (status != TOBII_RESEARCH_STATUS_OK)
    {
        _eyeTracker = nullptr;
        std::stringstream os;
        os << "connectToTobii: Cannot get eye tracker \"" << address_ << "\", ";
        os << "Error code: " << static_cast<int>(status);
        DoExitWithMsg(os.str());
    }
    return true;
}
bool UDPMultiCast::setTobiiSampleRate(float sampleFreq_)
{
    if (_eyeTracker)
    {
        TobiiResearchStatus status = tobii_research_set_gaze_output_frequency(_eyeTracker, sampleFreq_);
        if (status != TOBII_RESEARCH_STATUS_OK)
        {
            std::stringstream os;
            os << "connectToTobii: Cannot set sampling frequency to " << sampleFreq_ << ", ";
            os << "Error code: " << static_cast<int>(status);
            DoExitWithMsg(os.str());
        }
    }
    return true;
}
void UDPMultiCast::setTobiiScrSize(std::vector<double> scrSize_)
{
    if (scrSize_.size()!=2)
    {
        std::stringstream os;
        os << "setTobiiScrSize: Expected 2 values ([x,y] screen size), got " << scrSize_.size();
        DoExitWithMsg(os.str());
    }
    _scrSize = scrSize_;
}
bool UDPMultiCast::startTobiiDataSender()
{
    if (_eyeTracker)
        _tobiiDataSenderStarted = tobii_research_subscribe_to_gaze_data(_eyeTracker, TobiiSampleCallback, this) == TOBII_RESEARCH_STATUS_OK;
    else
        _tobiiDataSenderStarted = false;
    return _tobiiDataSenderStarted;
}
void UDPMultiCast::removeTobiiDataSender()
{
    if (_eyeTracker && _tobiiDataSenderStarted)
        tobii_research_unsubscribe_from_gaze_data(_eyeTracker, TobiiSampleCallback);
    else
        _tobiiDataSenderStarted = false;
}
#endif