#pragma once
#include "includes.h"
#include "str2int.h"

#include <string>
#include <sstream>
#include <limits>
#include <iostream>
#include <algorithm>

#include <VersionHelpers.h>



// the below function is called when an error occurred and application execution should halt
// this function is not defined in this library, it is for the user to implement depending on his platform
void DoExitWithMsg(std::string errMsg_);


inline std::string GetLastErrorMessage(
	DWORD last_error,
	bool stripTrailingLineFeed = true)
{
	CHAR errmsg[512];

	if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		0,
		last_error,
		0,
		errmsg,
		511,
		NULL))
	{
		// if we fail, call ourself to find out why and return that error

		const DWORD thisError = ::GetLastError();

		if (thisError != last_error)
		{
			return GetLastErrorMessage(thisError, stripTrailingLineFeed);
		}
		else
		{
			// But don't get into an infinite loop...

			return "Failed to obtain error string";
		}
	}

	if (stripTrailingLineFeed)
	{
		const size_t length = strlen(errmsg);

		if (errmsg[length - 1] == '\n')
		{
			errmsg[length - 1] = 0;

			if (errmsg[length - 2] == '\r')
			{
				errmsg[length - 2] = 0;
			}
		}
	}

	return errmsg;
}

inline void ErrorExit(
	const char *pFunction,
	const int lineNumber,
	const DWORD lastError)
{
	std::stringstream os;

	if (!lineNumber)
		os << "Error: " << pFunction << " failed: " << lastError << std::endl;
	else
		os << "Error: " << pFunction << " failed on line " << lineNumber << ": " << lastError << std::endl;
	os << GetLastErrorMessage(lastError) << std::endl;
	
	DoExitWithMsg(os.str());
}

inline void ErrorExit(
	const char *pFunction, const int lineNumber)
{
	const DWORD lastError = ::GetLastError();

	ErrorExit(pFunction, lineNumber, lastError);
}

inline void ErrorExit(
	const char *pFunction)
{
	ErrorExit(pFunction, 0);
}

inline void ErrorMsgExit(
	const char *pMsg_)
{
	std::stringstream os;
	os << pMsg_ << std::endl;

	DoExitWithMsg(os.str());
}

template <typename TV, typename TM>
inline TV RoundDown(TV Value, TM Multiple)
{
	return((Value / Multiple) * Multiple);
}

template <typename TV, typename TM>
inline TV RoundUp(TV Value, TM Multiple)
{
	return(RoundDown(Value, Multiple) + (((Value % Multiple) > 0) ? Multiple : 0));
}


inline LPVOID AllocateBufferSpace(
	const size_t bufferSize,
	const unsigned long nBuffers,
	DWORD &nBuffersAllocated)
{
	// with modern processors, there is normally only one NUMA node per socket,
	// which means the below NUMA stuff is of no relevance for our usage cases
	// might as well keep it though
	const DWORD preferredNumaNode = 0;

	SYSTEM_INFO systemInfo;

	::GetSystemInfo(&systemInfo);

	systemInfo.dwAllocationGranularity;

	const unsigned __int64 granularity = systemInfo.dwAllocationGranularity;

	const unsigned __int64 desiredSize = bufferSize * nBuffers;

	unsigned __int64 actualSize = RoundUp(desiredSize, granularity);

	if (actualSize > std::numeric_limits<DWORD>::max())
	{
		actualSize = (std::numeric_limits<DWORD>::max() / granularity) * granularity;
	}

	nBuffersAllocated = std::min<DWORD>(nBuffers, static_cast<DWORD>(actualSize / bufferSize));

	DWORD totalBufferSize = static_cast<DWORD>(actualSize);
	LPVOID pBuffer = VirtualAllocExNuma(GetCurrentProcess(), 0, totalBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE, preferredNumaNode);

	if (pBuffer == 0)
	{
		ErrorExit("VirtualAllocExNuma");
	}

	return pBuffer;
}

int64_t getTimeStamp();	// signed so we don't get in trouble if user does calculations with output that yield negative numbers