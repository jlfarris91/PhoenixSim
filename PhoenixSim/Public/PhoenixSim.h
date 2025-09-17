
#pragma once

#include <windows.h>
#include <string>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <cassert>
#include <functional>

#include "DLLExport.h"
#include "Name.h"

namespace Phoenix
{
    typedef std::string PHXString;

    template <class T> using TArray = std::vector<T>;
    template <class ...TArgs> using TTuple = std::tuple<TArgs...>;
    template <class TKey, class TValue> using TMap = std::map<TKey, TValue>;
    template <class T> using TDelegate = std::function<T>;

    template <class T> using TSharedPtr = std::shared_ptr<T>;
    template <class T> using TSharedAsThis = std::enable_shared_from_this<T>;
    template <class T> using TWeakPtr = std::weak_ptr<T>;

    typedef int8_t int8;
    typedef int16_t int16;
    typedef int32_t int32;
    typedef int64_t int64;
    typedef uint8_t uint8;
    typedef uint16_t uint16;
    typedef uint32_t uint32;
    typedef uint64_t uint64;

    typedef clock_t dt_t;
    typedef uint64 simtime_t;

    constexpr int32 INDEX_NONE = -1;

#define PHX_ASSERT(...) assert(__VA_ARGS__)
}
