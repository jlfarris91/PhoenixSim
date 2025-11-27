
#pragma once

#ifdef PHOENIX_DLL
    #ifdef _WIN32
        #ifdef PHOENIX_LDS_DLL_EXPORTS
            #define PHOENIX_LDS_API __declspec(dllexport)
        #else
            #define PHOENIX_LDS_API __declspec(dllimport)
        #endif
    #else
        // Linux/GCC: Use visibility attributes for shared libraries
        #ifdef PHOENIX_LDS_DLL_EXPORTS
            #define PHOENIX_LDS_API __attribute__((visibility("default")))
        #else
            #define PHOENIX_LDS_API
        #endif
    #endif
#else
    #define PHOENIX_LDS_API
#endif