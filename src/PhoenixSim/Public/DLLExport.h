
#pragma once

#ifdef _WIN32
    #ifdef PHOENIXSIM_DLL_EXPORTS
        #define PHOENIXSIM_API __declspec(dllexport)
    #else
        #define PHOENIXSIM_API __declspec(dllimport)
    #endif
#else
    // Linux/GCC: Use visibility attributes for shared libraries
    #ifdef PHOENIXSIM_DLL_EXPORTS
        #define PHOENIXSIM_API __attribute__((visibility("default")))
    #else
        #define PHOENIXSIM_API
    #endif
#endif
