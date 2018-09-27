#pragma once

//#define IP_ADDR_AS_STR

#if __has_include("iViewXAPI.h")
#   define HAS_SMI_INTEGRATION
#endif

#if __has_include("G_User.h")
#   define HAS_WINDOWSTIMESTAMPPROJECT
#endif

#if __has_include(<tobii_research.h>)
#   define HAS_TOBII_INTEGRATION
#endif