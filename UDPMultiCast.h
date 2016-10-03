#pragma once
#include "includes.h"
#include "types.h"
#include <vector>
#include <string>
#include <set>



class UDPMultiCast
{
public:
    UDPMultiCast();
    ~UDPMultiCast();

    void init();
    void deInit();
    void sendWithTimeStamp(const std::string msg_, const char delim_ = ',');
    void send(const std::string msg_);
    // check if receiver threads are still running. They may close when an exit message is received
    // function returns how many threads are running
    int  checkReceiverThreads();

    // get the data and command messages received since the last call to this function
    // these are distinguished by putting dat or cmd in front of each message
    std::vector<message> getData();
    std::vector<message> getCommands();

    // getters and setters
    // these can be called at any time
    std::string getGitRefID() const;	// implemented in cpp as frequent changes would trigger frequent complete recompiles...
    void setUseWTP(bool useWTP_);
    void setMaxClockRes(bool setMaxClockRes_);
    BOOL getLoopBack() const { return _loopBack; }
    void setLoopBack(const BOOL& loopBack_);
    std::string getGroupAddress() const { return _groupAddress; }
    void setGroupAddress(const std::string& groupAddress_);
    // for these, the setters can only be called before init is called
    unsigned short getPort() const { return _port; }
    void setPort(const unsigned short& port_);
    size_t getBufferSize() const { return _bufferSize; }
    void setBufferSize(const size_t& bufferSize_);
    unsigned long getNumQueuedReceives() const { return _IOCPPendingReceives; };
    void setNumQueuedReceives(const unsigned long& IOCPPendingReceives_);
    unsigned long getNumReceiverThreads() const { return _numIOCPThreads; };
    void setNumReceiverThreads(const unsigned long& numIOCPThreads_);
	void setComputerFilter(double* computerFilter_, size_t numElements_);

private:
    void sendInternal(EXTENDED_OVERLAPPED* sendOverlapped_);
    void receive(EXTENDED_OVERLAPPED* pExtOverlapped_, OVERLAPPED* pOverlapped = nullptr);
    void stopIOCP();
    void waitIOCPThreadsStop();
    static unsigned int __stdcall startThreadFunction(void *pV);
    unsigned int threadFunction();
	MsgType processMsg(const char* msg_, size_t *headerLen_, size_t *msgLen_);

    // for executing option changes
    void setupLoopBack(const BOOL loopBack_);
    void setupAndJoinMultiCast();
    void leaveMultiCast();


private:
    // state
    bool _initialized = false;
    SOCKET _socket;
    HANDLE _hIOCP;
    bool _haveCriticalSection = false;
    CRITICAL_SECTION _criticalSection;
    Threads _threads;
    bool _multiCastJoined = false;
    ip_mreq _multiCastRequest;
    sockaddr_in _multiCastGroup;
    // buffers
    char *_pReceiveBuffers;
    sockaddr_in *_pAddrBuffers;
    EXTENDED_OVERLAPPED *_pExtOverlappedArray;
    queue_t<4096> _receivedData;
    queue_t<512>  _receivedCommands;
	// filter for content to make it into the buffer
	bool _hasComputerFilter = false;
	std::set<char> _computerFilter;	// if not empty, only messages from computers in this set are processed

    // params and user prefs, with defaults
    bool _useWTP = false;
    bool _setMaxClockRes = false;
    DWORD _spinCount = 4000;
    // these can be set
    size_t _bufferSize = 4096;
    unsigned long _IOCPPendingReceives = 5000;
    unsigned long _numIOCPThreads = 8;	// doesn't hurt to have more than number of logical CPU cores, new threads only awakened if others blocked
    unsigned short _port = 10000;
    // more prefs, these can be changed after init
    std::string _groupAddress = "224.0.0.9";
    BOOL _loopBack = FALSE;

	// SMI data read integration
#ifdef HAS_SMI_INTEGRATION
public:
	void startSMIDataSender(bool needConnect = false);
	void removeSMIDataSender();
private:
	bool _smiDataSenderStarted = false;
#endif // DEBUG

};
// callback must be a free function
#ifdef HAS_SMI_INTEGRATION
#include "iViewXAPI.h"
int __stdcall SMISampleCallback(SampleStruct sampleData);
#endif // DEBUG