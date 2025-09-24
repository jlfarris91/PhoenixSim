
#pragma once

#include <cstdint>

#include "PhoenixSim.h"

namespace Phoenix
{
    constexpr uint32 MortonCodeGridBits = 6;

    // Expand a 32-bit integer into 64 bits by inserting 0s between the bits
    PHOENIXSIM_API constexpr uint64 ExpandBits(uint32_t v)
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
    PHOENIXSIM_API constexpr uint64 MortonCode(uint32_t x, uint32_t y)
    {
        return (ExpandBits(x) << 1) | ExpandBits(y);
    }

    struct PHOENIXSIM_API MortonCodeAABB
    {
        uint64 MinX = 0, MinY = 0;
        uint64 MaxX = 0, MaxY = 0;
    };

    PHOENIXSIM_API void MortonCodeQuery(
        const MortonCodeAABB& query,
        TArray<TTuple<uint64, uint64>>& outRanges,
        uint32 gridBits = MortonCodeGridBits);

    template <class T, uint64 T::*MemPtr, class TRange, class TPred>
    void ForEachInMortonCodeRanges(
        const TRange& sorted,
        const TArray<TTuple<uint64, uint64>>& ranges,
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
