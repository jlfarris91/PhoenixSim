
#pragma once

#ifdef PHOENIXSIM_DLL
#ifdef PHOENIXSIM_DLL_EXPORTS
#define PHOENIXSIM_API __declspec(dllexport)
#else
#define PHOENIXSIM_API __declspec(dllimport)
#endif
#else
#define PHOENIXSIM_API
#endif