
#pragma once

#ifdef PHOENIX_DLL
    #ifdef _WIN32
        #ifdef PHOENIXECS_DLL_EXPORTS
            #define PHOENIXECS_API __declspec(dllexport)
        #else
            #define PHOENIXECS_API __declspec(dllimport)
        #endif
    #else
        // Linux/GCC: Use visibility attributes for shared libraries
        #ifdef PHOENIXECS_DLL_EXPORTS
            #define PHOENIXECS_API __attribute__((visibility("default")))
        #else
            #define PHOENIXECS_API
        #endif
    #endif
#else
    #define PHOENIXECS_API
#endif