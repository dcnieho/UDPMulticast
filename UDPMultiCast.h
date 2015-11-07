#pragma once
#include "includes.h"
#include "types.h"
#include <vector>
#include <string>



class UDPMultiCast
{
public:
	UDPMultiCast();
	~UDPMultiCast();

	// set preferences before init or defaults will be used. 
	void init();
	void deInit();
	void sendWithTimeStamp(const std::string msg_, const char delim_ = ',');
	void send(const std::string msg_);

	// get the data and command messages received since the last call to this function
	std::vector<message> getData();
	std::vector<message> getCommands();

	// getters and setters
	// these can be called at any time
	BOOL getLoopBack() const { return _loopBack; }
	void setLoopBack(const BOOL& loopBack_);
	std::string getGroupAddress() const { return _groupAddress; }
	void setGroupAddress(const std::string& groupAddress_);
	// these can only be called before init is called
	unsigned short getPort() const { return _port; }
	void setPort(const unsigned short& port_);
	size_t getBufferSize() const { return _bufferSize; }
	void setBufferSize(const size_t& bufferSize_);
	unsigned long getNumQueuedReceives() const { return _IOCPPendingReceives; };
	void setNumQueuedReceives(const unsigned long& IOCPPendingReceives_);
	unsigned long getNumReceiverThreads() const { return _numIOCPThreads; };
	void setNumReceiverThreads(const unsigned long& numIOCPThreads_);

private:
	void sendInternal(EXTENDED_OVERLAPPED* sendOverlapped_);
	void receive(EXTENDED_OVERLAPPED* pExtOverlapped_, OVERLAPPED* pOverlapped = nullptr);
	void stopIOCP();
	void checkIOCPThreads();
	void waitIOCPThreadsStop();
	static unsigned int __stdcall startThreadFunction(void *pV);
	unsigned int threadFunction();
	MsgAction processMsg(const char* msg_, size_t *len_, size_t *msgLen_);

	// for executing option changes
	void setupLoopBack(const BOOL loopBack_);
	void setupAndJoinMultiCast();
	void leaveMultiCast();


private:
	// state
	bool _initialized = false;
	SOCKET _socket;
	HANDLE _hIOCP;
	CRITICAL_SECTION _criticalSection;
	Threads _threads;
	bool _multiCastJoined = false;
	ip_mreq _multiCastRequest;
	sockaddr_in _multiCastGroup;
	// buffers
	char *_pReceiveBuffers;	// allocated with VirtualAllocExNuma. TODO: why?
	EXTENDED_OVERLAPPED *_pExtOverlappedArray;
	queue_t<2048> _receivedData;
	queue_t<128>  _receivedCommands;

	// params and user prefs, with defaults
	DWORD _spinCount = 4000;
	// these can be set
	size_t _bufferSize = 4096;
	unsigned long _IOCPPendingReceives = 5000;
	unsigned long _numIOCPThreads = 8;	// doesn't hurt to have more than number of logical CPU cores, new threads only awakened if others blocked
	unsigned short _port = 10000;
	// more prefs, these can be changed after init
	std::string _groupAddress = "224.0.0.9";
	BOOL _loopBack = FALSE;
};