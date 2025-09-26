
#pragma once

#include "PlatformTypes.h"

namespace Phoenix
{
    // Represents a real number with fixed precision.
    // Tb is the bits of the denominator. A Tb value of 0 represents an integer.
    template <int64 Tb, class T = int32>
    struct TReal
    {
        static constexpr int64 TBitsMinusOne = ((sizeof(T) << 3) - 1);
        static_assert(Tb <= TBitsMinusOne);

        static constexpr int64 NB = TBitsMinusOne - Tb;
        static constexpr int64 DB = Tb;
        static constexpr T NM = (1 << NB) - 1;
        static constexpr T DM = (1 << DB) - 1;

        static constexpr T UnpackNum(T v) { return v >> Tb & NM; }
        static constexpr T UnpackDen(T v) { return v & DM; }

        template <class U> static constexpr T ToValue(U n, U d)
        {
            if constexpr (Tb == 0)
            {
                return n;
            }
            return ((n & NM) << Tb) | (d & DM);
        }

        template <class U> static constexpr U FromValue(T v)
        {
            if constexpr (Tb == 0)
            {
                return v;
            }
            return U((double)UnpackNum(v) / UnpackDen(v));
        }

        constexpr TReal() : Value(1) {}
        constexpr TReal(T n) : Value(ToValue(n, 1)) {}
        constexpr TReal(T n, T d) : Value(ToValue(n, d)) {}

        constexpr T Num() const { return UnpackNum(Value); }
        constexpr T Den() const { return UnpackDen(Value); }

        constexpr operator int32() const { return FromValue<int32>(Value); }
        constexpr operator uint32() const { return FromValue<uint32>(Value); }
        constexpr operator int64() const { return FromValue<int64>(Value); }
        constexpr operator uint64() const { return FromValue<uint64>(Value); }
        constexpr operator float() const { return FromValue<float>(Value); }
        constexpr operator double() const { return FromValue<double>(Value); }

        T Value = 1;
    };

    constexpr TReal<0> asdf(32, 2);
    constexpr double n = asdf.Num();
    constexpr double d = asdf.Den();
    constexpr double v = asdf;

    constexpr double aa = TReal<2>(2147483647);

    static_assert((double)TReal<16>(32, 2) == 16.0);
}
