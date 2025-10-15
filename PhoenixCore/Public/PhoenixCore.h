
#pragma once

#define NOMINMAX

#include <windows.h>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cassert>
#include <functional>
#include <unordered_set>

#include "DLLExport.h"
#include "Name.h"
#include "FixedPoint/FixedTypes.h"

#pragma warning( disable : 4251 )

namespace Phoenix
{
    typedef std::string PHXString;

    template <class T> using TArray = std::vector<T>;
    template <class T, class THasher> using TSet = std::unordered_set<T, THasher>;
    template <class ...TArgs> using TTuple = std::tuple<TArgs...>;
    template <class TKey, class TValue> using TMap = std::map<TKey, TValue>;
    template <class T> using TDelegate = std::function<T>;

    template <class T> using TSharedPtr = std::shared_ptr<T>;
    template <class T> using TSharedAsThis = std::enable_shared_from_this<T>;
    template <class T> using TWeakPtr = std::weak_ptr<T>;

    template <class T> using TAtomic = std::atomic<T>;

    template <class T, class ...TArgs>
    TSharedPtr<T> MakeShared(TArgs&&... args)
    {
        return std::make_shared<T>(std::forward<TArgs>(args)...);
    }

    typedef int64 dt_t;
    typedef uint64 simtime_t;

    template <class T>
    struct Index { static constexpr T None = -1; };

    constexpr int32 INDEX_NONE = Index<int32>::None;

#define PHX_ASSERT(...) assert(__VA_ARGS__)

#ifndef PHX_CONCAT
#   define PHX_CONCAT(x, y) PHX_CONCAT_INDIRECT(x, y)
#endif

#ifndef PHX_CONCAT_INDIRECT
#   define PHX_CONCAT_INDIRECT(x, y) x##y
#endif

#ifndef PHX_FILE
#   define PHX_FILE __FILE__
#endif

#ifndef PHX_LINE
#   define PHX_LINE PHX_CONCAT(__LINE__, U)
#endif

#ifndef PHX_FUNCTION
#   define PHX_FUNCTION __FUNCTION__
#endif

#if defined(_MSC_VER)
#   define PHX_FORCE_INLINE __forceinline
#else
#   define PHX_FORCE_INLINE inline
#endif
}
