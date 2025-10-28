
#pragma once

#include <tuple>
#include <type_traits>

namespace Phoenix
{
    template <size_t I = 0, typename... Ts>
    std::enable_if_t<I == sizeof...(Ts), bool> ContainsNullptr(const std::tuple<Ts...>&)
    {
        return false;
    }

    template <size_t I = 0, typename... Ts>
    std::enable_if_t<I < sizeof...(Ts), bool> ContainsNullptr(const std::tuple<Ts...>& t)
    {
        using T = std::tuple_element_t<I, std::tuple<Ts...>>;
        if constexpr (std::is_pointer_v<T>)
        {
            if (std::get<I>(t) == nullptr)
            {
                return true;
            }
        }
        return ContainsNullptr<I + 1>(t);
    }
}
