#pragma once
#include "includes.h"
#include "mpmcBoundedQueue.h"

#include <deque>

struct EXTENDED_OVERLAPPED : public OVERLAPPED
{
	WSABUF buf;
	sockaddr addr;
	int addrLen;
	BOOL isRead;
};

typedef std::deque<HANDLE> Threads;

struct message
{
	char* text;
	int64_t timeStamp;
	// source ip
};
template <size_t buffer_size>
using queue_t = mpmc_bounded_queue<message,buffer_size>;

enum class MsgAction
{
	noAction = 0,
	exit,
	storeData,
	storeCommand
};