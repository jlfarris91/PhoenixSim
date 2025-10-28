
#pragma once

#ifdef PHOENIXLUA_DLL_EXPORTS
#define PHOENIXLUA_API __declspec(dllexport)
#else
#define PHOENIXLUA_API __declspec(dllimport)
#endif