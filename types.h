#pragma once
#include "includes.h"
#include "mpmcBoundedQueue.h"

#include <deque>
#include <cstring>

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

	// need comparison operator for boost.python objects contained in vectors
	// see http://stackoverflow.com/questions/10680691/why-do-i-need-comparison-operators-in-boost-python-vector-indexing-suite
	bool message::operator ==(const message &b) const
	{
		return timeStamp == b.timeStamp && strcmp(text, b.text);
	};
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