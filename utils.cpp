#include "utils.h"


static bool gtsInitialized = false;
static bool win8OrGreater = false;
typedef WINBASEAPI VOID (WINAPI *GETPRECISETIMEFUN)(_Out_ LPFILETIME);
static GETPRECISETIMEFUN GetPreciseTime = nullptr;

int64_t getTimeStamp()	// signed so we don't get in trouble if user does calculations with output that yield negative numbers
{
	// alternatively could pull out the big gun and use windows timestamp project
	// think what is below is good enough though...

	if (!gtsInitialized)
	{
		win8OrGreater = IsWindows8OrGreater();
		if (win8OrGreater)
		{
			HMODULE hMod = GetModuleHandle("kernel32");
			GetPreciseTime = (GETPRECISETIMEFUN)GetProcAddress(hMod, "GetSystemTimePreciseAsFileTime");
		}

		// done with init
		gtsInitialized = true;
	}

	if (win8OrGreater)
	{
		FILETIME ft;
		(*GetPreciseTime)(&ft);	// GetSystemTimePreciseAsFileTime

		// Windows file times are in 100s of nanoseconds.
		// Convert to microseconds by dividing by 10.

		// Then convert to Unix epoch:
		// Between January 1, 1601 and January 1, 1970, there were 369 complete years,
		// of which 89 were leap years (1700, 1800, and 1900 were not leap years).
		// That is a total of 134774 days, which is 11644473600 seconds.
		return ((static_cast<int64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime) / 10 - 11644473600 * 100000;
	}
	else
	{
		// two alternatives for using QPC to provide high resolution part to system time:
		// http://www.opensource.apple.com/source/JavaScriptCore/JavaScriptCore-7534.57.3/wtf/CurrentTime.cpp
		// http://stackoverflow.com/a/29937446/3103767. I chose the second as the way to sync low res and highres time seems more sensible
		
		static LARGE_INTEGER    uFrequency = { 0 };
		static LARGE_INTEGER    uInitialCount;
		static LARGE_INTEGER    uLastCount;
		static LARGE_INTEGER    uInitialTime;

		if (uFrequency.QuadPart == 0)
		{
			// Initialize performance counter to system time mapping
			if (!QueryPerformanceFrequency(&uFrequency))
				ErrorExit("QueryPerformanceFrequency");

			FILETIME ftOld, ftInitial;

			// spin until low resolution clock ticks, then we have a value for the
			// high resolution counter as close as possible to this tick without
			// more complicated calibration routines
			GetSystemTimeAsFileTime(&ftOld);
			do
			{
				GetSystemTimeAsFileTime(&ftInitial);
				QueryPerformanceCounter(&uInitialCount);
			} while (ftOld.dwHighDateTime == ftInitial.dwHighDateTime && ftOld.dwLowDateTime == ftInitial.dwLowDateTime);
			uInitialTime.LowPart = ftInitial.dwLowDateTime;
			uInitialTime.HighPart = ftInitial.dwHighDateTime;
			uLastCount = uInitialCount;
		}

		
		LARGE_INTEGER   uNow, uSystemTime;
		FILETIME    ft;
		GetSystemTimeAsFileTime(&ft);

		// query current value of tick counter
		QueryPerformanceCounter(&uNow);
		LARGE_INTEGER   uCurrentTime;
		// use high frequency counter to calculate time elapsed since init, add to time of init to get current time
		auto t1 = uNow.QuadPart - uInitialCount.QuadPart;
		if (uNow.QuadPart - uLastCount.QuadPart <= 0 || t1 <= 0)
		{
			// got shitty time (changed core?). live with it and provide lowres output
			// as above
			return ((static_cast<int64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime) / 10 - 11644473600 * 100000;
		}
		else
			uLastCount = uNow;

		uCurrentTime.QuadPart = uInitialTime.QuadPart + t1 * 10000000 / uFrequency.QuadPart;

		// Check if the performance counter has been frozen (e.g. after standby on laptops)
		// -> Use lowres system time and determine the high performance time the next time we need it
		uSystemTime.LowPart = ft.dwLowDateTime;
		uSystemTime.HighPart = ft.dwHighDateTime;
		if (uCurrentTime.QuadPart < uSystemTime.QuadPart || uCurrentTime.QuadPart - uSystemTime.QuadPart > 1000000)
		{
			uFrequency.QuadPart = 0;	// we'll init again on next run
			uCurrentTime = uSystemTime;
		}

		// all good, return!
		return uCurrentTime.QuadPart / 10 - 11644473600 * 100000;
	}
}