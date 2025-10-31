
#pragma once

#include <cstdint>

namespace Phoenix
{
    typedef int8_t int8;
    typedef int16_t int16;
    typedef int32_t int32;
    typedef int64_t int64;
    typedef uint8_t uint8;
    typedef uint16_t uint16;
    typedef uint32_t uint32;
    typedef uint64_t uint64;
}

#ifdef PHOENIX_DLL
    #ifdef _WIN32
        #ifdef PHOENIXCORE_DLL_EXPORTS
            #define PHOENIXCORE_API __declspec(dllexport)
        #else
            #define PHOENIXCORE_API __declspec(dllimport)
        #endif
    #else
        // Linux/GCC: Use visibility attributes for shared libraries
        #ifdef PHOENIXCORE_DLL_EXPORTS 
            #define PHOENIXCORE_API __attribute__((visibility("default")))
        #else
            #define PHOENIXCORE_API
        #endif
    #endif
#else
    #define PHOENIXCORE_API
#endif

#ifdef _WIN32
    #define PHX_FORCEINLINE __forceinline
    #define PHX_THREAD_PAUSE _mm_pause()
#else
    // Linux/GCC
    #define PHX_FORCEINLINE inline __attribute__((always_inline))
    #define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
    #define PHX_THREAD_PAUSE nanosleep()
#endif