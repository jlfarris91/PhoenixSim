
#pragma once

#include <cstdint>

#include "PhoenixCore.h"
#include "PlatformTypes.h"
#include "FixedPoint/FixedVector.h"

namespace Phoenix
{
    constexpr uint32 MortonCodeGridBits = 6;
    using TMortonCodeRangeArray = TArray<TTuple<uint64, uint64>>;

    // Expand a 32-bit integer into 64 bits by inserting 0s between the bits
    constexpr uint64 ExpandBits(uint32_t v)
    {
        uint64 x = v;
        x = (x | (x << 16)) & 0x0000FFFF0000FFFFULL;
        x = (x | (x << 8))  & 0x00FF00FF00FF00FFULL;
        x = (x | (x << 4))  & 0x0F0F0F0F0F0F0F0FULL;
        x = (x | (x << 2))  & 0x3333333333333333ULL;
        x = (x | (x << 1))  & 0x5555555555555555ULL;
        return x;
    }

    // Create Morton code from 2D coordinates
    constexpr uint64 MortonCode(uint32_t x, uint32_t y)
    {
        // uint32 xx = x ^ 0x80000000;
        // uint32 yy = y ^ 0x80000000;
        return (ExpandBits(x) << 1) | ExpandBits(y);
    }

    struct MortonCodeAABB
    {
        uint32 MinX = 0, MinY = 0;
        uint32 MaxX = 0, MaxY = 0;
    };

    MortonCodeAABB ToMortonCodeAABB(Vec2 pos, Distance radius);

    void MortonCodeQuery(
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
                predicate(*itr);
            }
        }
    }
}
