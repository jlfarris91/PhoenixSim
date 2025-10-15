
#pragma once

#include <cstdint>

#include "PhoenixCore.h"
#include "PlatformTypes.h"
#include "FixedPoint/FixedVector.h"

namespace Phoenix
{
    constexpr uint32 MortonCodeGridBits = Distance::B >> 2;
    using TMortonCodeRangeArray = TArray<TTuple<uint64, uint64>>;

    // Expand a 32-bit integer into 64 bits by inserting 0s between the bits
    PHOENIXCORE_API constexpr uint64 ExpandBits(uint32 v)
    {
        uint64 x = v;
        x = (x | (x << 16)) & 0x0000FFFF0000FFFFULL;
        x = (x | (x << 8))  & 0x00FF00FF00FF00FFULL;
        x = (x | (x << 4))  & 0x0F0F0F0F0F0F0F0FULL;
        x = (x | (x << 2))  & 0x3333333333333333ULL;
        x = (x | (x << 1))  & 0x5555555555555555ULL;
        return x;
    }

    // Collapse an expanded 64-bit integer back into 32 bits by removing 0s between the bits
    PHOENIXCORE_API constexpr uint32 CollapseBits(uint64 v)
    {
        uint64 x = v;
        x &= 0x5555555555555555ULL;
        x = (x ^ (x >> 1)) & 0x3333333333333333ULL;
        x = (x ^ (x >> 2)) & 0x0F0F0F0F0F0F0F0FULL;
        x = (x ^ (x >> 4)) & 0x00FF00FF00FF00FFULL;
        x = (x ^ (x >> 8)) & 0x0000FFFF0000FFFFULL;
        return static_cast<int32>(x);
    }

    // Create Morton code from 2D coordinates
    PHOENIXCORE_API constexpr uint64 ToMortonCode(uint32 x, uint32 y, uint8 lshift = MortonCodeGridBits)
    {
        // uint32 xx = x ^ 0x80000000;
        // uint32 yy = y ^ 0x80000000;
        return (ExpandBits(x >> lshift) << 1) | ExpandBits(y >> lshift);
    }

    PHOENIXCORE_API constexpr uint8 GetMortonCodeQuad(uint64 zcode)
    {
        return zcode >> 61;
    }

    PHOENIXCORE_API constexpr uint64 GetMortonCodeValue(uint64 zcode)
    {
        constexpr uint64 valueMask = ~(uint64(0x7) << 61); 
        return zcode & valueMask;
    }

    // Create Morton code from signed 2D coordinates
    // The top 3 bits represent which quadrant the coordinate is in.
    // Quadrant 0: +x, +y
    // Quadrant 1: -x, +y
    // Quadrant 2: -x, -y
    // Quadrant 3: +x, -y
    PHOENIXCORE_API constexpr uint64 ToMortonCode(int32 x, int32 y, uint8 lshift = MortonCodeGridBits)
    {
        uint64 quad = 0;
        if (x < 0) quad |= 1;
        if (y < 0) quad |= 2;
        uint32 xu = x < 0 ? -x - 1 : x;
        uint32 yu = y < 0 ? -y - 1 : y;
        return ToMortonCode(xu, yu, lshift) | (quad << 61);
    }

    PHOENIXCORE_API constexpr uint64 ToMortonCode(Distance x, Distance y)
    {
        return ToMortonCode(int32(x), int32(y), MortonCodeGridBits);
    }

    PHOENIXCORE_API constexpr uint64 ToMortonCode(const Vec2& v)
    {
        return ToMortonCode(v.X, v.Y);
    }

    PHOENIXCORE_API constexpr void FromMortonCode(uint64 zcode, uint32& outX, uint32& outY, uint8 rshift = MortonCodeGridBits)
    {
        outX = CollapseBits(zcode >> 1) << rshift;
        outY = CollapseBits(zcode) << rshift;
    }

    PHOENIXCORE_API constexpr void FromMortonCode(uint64 zcode, int32& outX, int32& outY, uint8 rshift = MortonCodeGridBits)
    {
        uint8 quad = GetMortonCodeQuad(zcode);
        uint64 value = GetMortonCodeValue(zcode);
        uint32 x = (CollapseBits(value >> 1) + ((quad & 1) ? 1 : 0)) << rshift;
        uint32 y = (CollapseBits(value) + ((quad & 2) ? 1 : 0)) << rshift;
        outX = int32(x) * ((quad & 1) ? -1 : 1);
        outY = int32(y) * ((quad & 2) ? -1 : 1);
    }

    PHOENIXCORE_API constexpr void FromMortonCode(uint64 zcode, Distance& outX, Distance& outY, uint8 rshift = MortonCodeGridBits)
    {
        int32 x, y;
        FromMortonCode(zcode, x, y, rshift);
        outX = Distance(x);
        outY = Distance(y);
    }

    PHOENIXCORE_API constexpr Vec2 FromMortonCode(uint64 zcode, uint8 rshift = MortonCodeGridBits)
    {
        int32 x, y;
        FromMortonCode(zcode, x, y, rshift);
        return { x, y };
    }

    PHOENIXCORE_API constexpr int32 FromMortonCodeX(uint64 zcode, uint8 rshift = MortonCodeGridBits)
    {
        uint8 quad = GetMortonCodeQuad(zcode);
        uint64 value = GetMortonCodeValue(zcode);
        uint32 x = CollapseBits(value >> 1) << rshift;
        return int32(x) * ((quad & 1) ? -1 : 1);
    }

    PHOENIXCORE_API constexpr Distance FromMortonCodeX_Dist(uint64 zcode)
    {
        return FromMortonCodeX(zcode);
    }

    PHOENIXCORE_API constexpr int32 FromMortonCodeY(uint64 zcode, uint8 rshift = MortonCodeGridBits)
    {
        uint8 quad = GetMortonCodeQuad(zcode);
        uint64 value = GetMortonCodeValue(zcode);
        uint32 y = CollapseBits(value) << rshift;
        return int32(y) * ((quad & 2) ? -1 : 1);
    }

    PHOENIXCORE_API constexpr Distance FromMortonCodeY_Dist(uint64 zcode)
    {
        return FromMortonCodeY(zcode);
    }

    // Scales down to morton code space and reserves sign.
    PHOENIXCORE_API constexpr int32 ScaleToMortonCode(int32 x)
    {
        int8 sign = x >= 0 ? 1 : -1; 
        return (x * sign >> MortonCodeGridBits) * sign;
    }

    struct PHOENIXCORE_API MortonCodeAABB
    {
        int32 MinX = 0, MinY = 0;
        int32 MaxX = 0, MaxY = 0;
    };

    PHOENIXCORE_API MortonCodeAABB ToMortonCodeAABB(Vec2 pos, Distance radius);

    PHOENIXCORE_API void MortonCodeQuery(
        const MortonCodeAABB& query,
        TMortonCodeRangeArray& outRanges,
        uint32 gridBits = MortonCodeGridBits);

    template <class T, uint64 T::*MemPtr, class TRange, class TPred>
    void ForEachInMortonCodeRanges(
        const TRange& sorted,
        const TMortonCodeRangeArray& ranges,
        const TPred& predicate)
    {
        for (auto && [min, max] : ranges)
        {
            auto itrLo = std::lower_bound(sorted.begin(), sorted.end(), min, [](auto const& a, auto v)
            {
                return a.*MemPtr < v;
            });
            auto itrHi = std::upper_bound(sorted.begin(), sorted.end(), max, [](auto v, auto const& a)
            {
                return v < a.*MemPtr;
            });
            for (auto itr = itrLo; itr != itrHi; ++itr)
            {
                if constexpr(std::is_same_v<decltype(predicate(std::declval<decltype(*sorted.begin())>())), bool>)
                {
                    if (predicate(*itr))
                        break;
                }
                else
                {
                    predicate(*itr);
                }
            }
        }
    }
}
