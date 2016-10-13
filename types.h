#pragma once
#include "includes.h"
#include "mpmcBoundedQueue.h"

#include <deque>
#include <cstring>
#include <memory>

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
    message() :
        text({nullptr,std::free})
    {}
#ifdef IP_ADDR_AS_STR
    static_assert(false,"to implement");
#else
    message(unsigned char ip_, char* text_, int64_t timeStamp_) :
        ip(ip_),
        text({text_,std::free}),
        timeStamp(timeStamp_)
    {}
#endif
    // move ctor
    message(message&&) = default;
    // move assignment
    message& operator= (message&&) = default;
    // dtor
    ~message() = default;
    // copy ctor and assignment are implicitly deleted (and we have a unique_ptr member)

    // need comparison operator for boost.python objects contained in vectors
    // see http://stackoverflow.com/questions/10680691/why-do-i-need-comparison-operators-in-boost-python-vector-indexing-suite
    bool message::operator ==(const message &b) const
    {
#ifdef IP_ADDR_AS_STR
        return timeStamp == b.timeStamp && !strcmp(text.get(), b.text.get()) && !strcmp(ip, b.ip);  // strcmp outputs 0 when inputs identical
#else
        return timeStamp == b.timeStamp && !strcmp(text.get(), b.text.get()) && ip==b.ip;           // strcmp outputs 0 when inputs identical
#endif
    };

    // member variables
    std::unique_ptr<char, decltype(std::free)*> text;
    int64_t timeStamp;	// receive timestamp
#ifdef IP_ADDR_AS_STR
    char  ip[INET_ADDRSTRLEN] = { '\0' };	// source ip
#else
    unsigned char  ip;	// last byte of IP
#endif
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