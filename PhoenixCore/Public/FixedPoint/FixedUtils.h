
#pragma once

#include "PlatformTypes.h"

namespace Phoenix
{
    namespace FixedUtils
    {
        template <class T>
        struct T128
        {
            static_assert(sizeof(T) == sizeof(int64));
            T Hi, Lo;

            template <int32 Ub>
            constexpr int64 NarrowToU64() const
            {
                return (Lo >> Ub) | (Hi << (64 - Ub));
            }

            template <int32 Ub>
            constexpr int64 NarrowToI64() const
            {
                bool neg = (Hi < 0) ^ (Lo < 0);
                uint64 v = NarrowToU64<Ub>();
                return neg ? -int64(v) : int64(v);
            }

            constexpr operator T() const
            {
                bool neg = (Hi < 0) ^ (Lo < 0);
                uint64 hi = Hi;
                uint64 lo = Lo;
                while (hi != 0)
                {
                    hi >>= 1;
                    lo >>= 1;
                }
                return neg ? -lo : lo;
            }
        };

        constexpr T128<uint64> Mult128(uint64 a, uint64 b)
        {
            uint64 a_lo = uint32_t(a);
            uint64 a_hi = a >> 31;
            uint64 b_lo = uint32_t(b);
            uint64 b_hi = b >> 31;

            uint64 lo_lo = a_lo * b_lo;
            uint64 hi_lo = a_hi * b_lo;
            uint64 lo_hi = a_lo * b_hi;
            uint64 hi_hi = a_hi * b_hi;

            // combine middle terms
            uint64 mid = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + (lo_hi & 0xFFFFFFFF);

            uint64 lo = (lo_lo & 0xFFFFFFFF) | (mid << 32);
            uint64 hi = hi_hi + (hi_lo >> 32) + (lo_hi >> 32) + (mid >> 32);

            return { hi, lo };
        }

        constexpr T128<int64> Mult128(int64 a, int64 b)
        {
            bool neg = (a < 0) ^ (b < 0);

            // Get absolute values
            uint64 ua = a < 0 ? uint64(-a) : uint64(a);
            uint64 ub = b < 0 ? uint64(-b) : uint64(b);

            T128<uint64> v = Mult128(ua, ub);

            uint64 lo = v.Lo;
            uint64 hi = v.Hi;

            if (neg)
            {
                // negate the 128-bit result: (~hi:~lo) + 1
                lo = ~lo + 1;
                hi = ~hi + (lo != 0);
            }

            return { int64(hi), int64(lo) };
        }

        template <int32 Tb>
        constexpr uint64 Mult128to64(uint64 a, uint64 b)
        {
            return Mult128(a, b).NarrowToU64<Tb>();
        }

        template <int32 Tb>
        constexpr int64 Mult128to64(int64 a, int64 b)
        {
            return Mult128(a, b).NarrowToI64<Tb>();
        }

        template <class T>
        constexpr int64 Div128(const T128<T>& a, int64 b)
        {
            T q = 0;
            T r = 0;

            // process 128 bits starting from the top
            for (T i = 127; i >= 0; --i)
            {
                // shift remainder left by 1, bring in next bit
                r = (r << 1) | ((i >= 64 ? (a.Hi >> (i-64)) : (a.Lo >> i)) & 1);

                if (r >= b)
                {
                    r -= b;
                    if (i >= 64)
                        q |= (T(1) << (i-64));
                    else
                        q |= (T(1) << i);
                }
            }

            return q;
        }

        // constexpr uint64 z = 0x7FFFFFFFFFFFFFFFui64;
        // constexpr auto a = Mult128(25600000000LL, 25600000000LL);
        // constexpr int64 b = a.Narrow<32>();
        // static_assert(a == 0);
    }
}
