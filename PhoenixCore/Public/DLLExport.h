
#pragma once

#ifdef PHOENIXCORE_DLL_EXPORTS
#define PHOENIXCORE_API __declspec(dllexport)
#else
#define PHOENIXCORE_API __declspec(dllimport)
#endif