
#pragma once

#include <type_traits>

namespace Phoenix
{
    template <class T, class U>
    constexpr bool HasAnyFlags(T flags, U value)
    {
        using V = std::underlying_type_t<T>;
        return (static_cast<V>(flags) & static_cast<V>(value)) != 0;
    }

    template <class T, class ...Us>
    constexpr bool HasAnyFlags(T flags, Us&& ...args)
    {
        return (HasAnyFlags<T, Us>(flags, args) && ...);
    }

    template <class T, class U>
    constexpr bool HasAllFlags(T flags, U value)
    {
        using V = std::underlying_type_t<T>;
        return (static_cast<V>(flags) & static_cast<V>(value)) == static_cast<V>(value);
    }

    template <class T, class ...Us>
    constexpr bool HasAllFlags(T flags, Us&& ...args)
    {
        return (HasAllFlags<T, Us>(flags, args) && ...);
    }

    template <class T, class U>
    constexpr bool HasNoneFlags(T flags, U value)
    {
        using V = std::underlying_type_t<T>;
        return (static_cast<V>(flags) & static_cast<V>(value)) == 0;
    }

    template <class T, class ...Us>
    constexpr bool HasNoneFlags(T flags, Us&& ...args)
    {
        return (HasNoneFlags<T, Us>(flags, args) && ...);
    }

    template <class T, class U>
    constexpr T SetFlag(T flags, U value, bool set)
    {
        using V = std::underlying_type_t<T>;
        if (set)
        {
            return static_cast<T>(static_cast<V>(flags) | static_cast<V>(value));
        }

        return static_cast<T>(static_cast<V>(flags) & ~static_cast<V>(value));
    }

    template <class T, class U>
    constexpr void SetFlagRef(T& flags, U value, bool set)
    {
        flags = SetFlag(flags, value, set);
    }
}
