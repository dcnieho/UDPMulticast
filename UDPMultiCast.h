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
	void sendWithTimeStamp(const std::string msg_, const char delim_);
	void send(const std::string msg_);

	// getters and setters
	BOOL getLoopBack() const { return _loopBack; }
	void setLoopBack(const BOOL& loopBack_);
	std::string getGroupAddress() const { return _groupAddress; }
	void setGroupAddress(const std::string& groupAddress_);
	unsigned short getPort() const { return _port; }
	void setPort(const unsigned short& port_);
	std::vector<message> getData();
	std::vector<message> getCommands();

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

	// user prefs, with defaults
	size_t _bufferSize = 4096;
	DWORD _IOCPPendingReceives = 5000;
	DWORD _numIOCPThreads = 8;	// doesn't hurt to have more than number of logical CPU cores, new threads only awakened if others blocked
	DWORD _spinCount = 4000;
	// more prefs, these can be changed after init (TODO)
	unsigned short _port = 10000;
	std::string _groupAddress = "224.0.0.9";
	BOOL _loopBack = FALSE;
};