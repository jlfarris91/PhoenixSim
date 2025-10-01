
#pragma once

#define NOMINMAX

#include <windows.h>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cassert>
#include <functional>

#include "Name.h"
#include "FixedPoint/FixedTypes.h"

#pragma warning( disable : 4251 )

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

    typedef int64 dt_t;
    typedef uint64 simtime_t;

    template <class T>
    struct Index { static constexpr T None = -1; };

    constexpr int32 INDEX_NONE = Index<int32>::None;

#define PHX_ASSERT(...) assert(__VA_ARGS__)
}
