
#pragma once

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cassert>
#include <functional>
#include <unordered_set>

#pragma warning( disable : 4251 )

#ifdef PHOENIX_DLL
    #ifdef _WIN32
        #ifdef PHOENIXCORE_DLL_EXPORTS
            #define PHOENIXCORE_API __declspec(dllexport)
        #else
            #define PHOENIXCORE_API __declspec(dllimport)
        #endif
    #else
        // Linux/GCC: Use visibility attributes for shared libraries
        #ifdef PHOENIXCORE_DLL_EXPORTS 
            #define PHOENIXCORE_API __attribute__((visibility("default")))
        #else
            #define PHOENIXCORE_API
        #endif
    #endif
#else
    #define PHOENIXCORE_API
#endif

#ifdef _WIN32
    #define PHX_FORCEINLINE __forceinline
    #define PHX_THREAD_PAUSE() _mm_pause()
    #define PHX_CLOCK() clock()
#else
    // Linux/GCC
    #define PHX_FORCEINLINE inline __attribute__((always_inline))
    #define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
    #define PHX_THREAD_PAUSE() { do {} while(0); }
    #define PHX_CLOCK() std::chrono::system_clock::now().time_since_epoch().count()
#endif

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

namespace Phoenix
{
    typedef int8_t int8;
    typedef int16_t int16;
    typedef int32_t int32;
    typedef int64_t int64;
    typedef uint8_t uint8;
    typedef uint16_t uint16;
    typedef uint32_t uint32;
    typedef uint64_t uint64;

    typedef std::string PHXString;

    template <class T> using TArray = std::vector<T>;
    template <class T, class THasher = std::hash<T>> using TSet = std::unordered_set<T, THasher>;
    template <class ...TArgs> using TTuple = std::tuple<TArgs...>;
    template <class TKey, class TValue> using TMap = std::map<TKey, TValue>;
    template <class T> using TDelegate = std::function<T>;

    template <class T> using TSharedPtr = std::shared_ptr<T>;
    template <class T> using TSharedAsThis = std::enable_shared_from_this<T>;
    template <class T> using TWeakPtr = std::weak_ptr<T>;
    template <class T> using TUniquePtr = std::unique_ptr<T>;

    template <class T>
    using TFunction = std::function<T>;

    template <class T> using TAtomic = std::atomic<T>;

    template <class T, class ...TArgs, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    TSharedPtr<T> MakeShared(TArgs&&... args)
    {
        return std::make_shared<T>(std::forward<TArgs>(args)...);
    }

    template <class T, std::enable_if_t<std::is_array_v<T> && std::extent_v<T> == 0, int> = 0>
    TSharedPtr<T> MakeShared(const size_t size)
    {
        return std::make_shared<T>(size);
    }

    template <class T, class ...TArgs, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    TUniquePtr<T> MakeUnique(TArgs&&... args)
    {
        return std::make_unique<T>(std::forward<TArgs>(args)...);
    }

    template <class T, std::enable_if_t<std::is_array_v<T> && std::extent_v<T> == 0, int> = 0>
    TUniquePtr<T> MakeUnique(const size_t size)
    {
        return std::make_unique<T>(size);
    }

    template <class T>
    PHX_FORCEINLINE T&& Forward(std::remove_reference_t<T>& obj)
    {
        return static_cast<T&&>(obj);
    }

    template <class T>
    PHX_FORCEINLINE T&& Forward(std::remove_reference_t<T>&& obj)
    {
        return static_cast<T&&>(obj);
    }

    template <class T>
    PHX_FORCEINLINE constexpr std::remove_reference_t<T>&& MoveTemp(T&& arg)
    {
        return std::move<T>(arg);
    }

    template <class T>
    struct Underlying
    {
        using type = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
    };

    template <class T>
    using Underlying_T = typename Underlying<T>::type;

    typedef int64 dt_t;
    typedef uint64 simtime_t;

    template <class T>
    struct Index { static constexpr T None = -1; };

    constexpr int32 INDEX_NONE = Index<int32>::None;
}