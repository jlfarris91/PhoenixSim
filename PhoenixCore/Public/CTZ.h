
#pragma once

#include <bit>

namespace Phoenix
{
    template <class T>
    constexpr T CTZ(T x)
    {
        return std::countr_zero(x);
    }

    template <class T>
    constexpr T CLZ(T x)
    {
        return std::countl_zero(x);
    }

    constexpr int32 RoundUpPowerOf2(int32 x)
    {
        if (x <= 1) return 1;
        return 1 << (32 - (int32)CLZ((uint32)x - 1));
    }
}