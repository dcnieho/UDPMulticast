#pragma once

//#define IP_ADDR_AS_STR

// use the 0 or 1 at the end of the each below statement to forcefully switch off inclusion of certain functionality in the compilation

#if __has_include("iViewXAPI.h") && 0
#   define HAS_SMI_INTEGRATION
#endif

#if __has_include("G_User.h") && 1
#   define HAS_WINDOWSTIMESTAMPPROJECT
#endif

#if __has_include(<tobii_research.h>) && 1
#   define HAS_TOBII_INTEGRATION
#endif