#pragma once
#include "includes.h"
#include "mpmcBoundedQueue.h"

#include <deque>
#include <cstring>

struct EXTENDED_OVERLAPPED : public OVERLAPPED
{
    WSABUF buf;
    sockaddr_in* addr = nullptr;
    int addrLen;
    BOOL isRead;
};

typedef std::deque<HANDLE> Threads;

class message
{
public:
    ~message()
    {
        if (text)
            delete[] text;
    }

    char* text;
    int64_t timeStamp;	// receive timestamp
#ifdef IP_ADDR_AS_STR
    char  ip[INET_ADDRSTRLEN] = { '\0' };	// source ip
#else
	unsigned char  ip;	// last byte of IP
#endif

    // need comparison operator for boost.python objects contained in vectors
    // see http://stackoverflow.com/questions/10680691/why-do-i-need-comparison-operators-in-boost-python-vector-indexing-suite
    bool message::operator ==(const message &b) const
    {
#ifdef IP_ADDR_AS_STR
        return timeStamp == b.timeStamp && !strcmp(text, b.text) && !strcmp(ip, b.ip);	// strcmp outputs 0 when inputs identical
#else
		return timeStamp == b.timeStamp && !strcmp(text, b.text) && ip==b.ip;			// strcmp outputs 0 when inputs identical
#endif
    };
};
template <size_t buffer_size>
using queue_t = mpmc_bounded_queue<message,buffer_size>;

enum class MsgType
{
    unknown = 0,
    exit,
    data,
    command
};