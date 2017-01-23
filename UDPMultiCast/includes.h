#pragma once

#include "UDPMultiCast/config.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <WinSDKVer.h>
#define _WIN32_WINNT 0x0601 // windows 7
#include <sdkddkver.h>

#include <tchar.h>

#include <WinSock2.h>
#include <MSWsock.h>


#include <WS2tcpip.h>