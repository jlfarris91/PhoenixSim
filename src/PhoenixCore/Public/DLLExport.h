
#pragma once

#ifdef _WIN32
    #ifdef PHOENIXCORE_DLL_EXPORTS
    #define PHOENIXCORE_API __declspec(dllexport)
    #else
    #define PHOENIXCORE_API __declspec(dllimport)
    #endif
    #define FORCEINLINE __forceinline
#else
    // Linux/GCC
    #define PHOENIXCORE_API __attribute__((visibility("default")))
    #define FORCEINLINE inline __attribute__((always_inline))
    #define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif
