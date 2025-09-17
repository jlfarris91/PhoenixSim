
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

    // constexpr void MortonCodeQuery(
    //     const MortonCodeAABB& query,
    //     const MortonCodeNode& node,
    //     TArray<TTuple<uint64, uint64>>& outRanges,
    //     uint32 level = 0)
    // {
    //     constexpr uint64 MAX_LEVEL = 10;
    //
    //     // Compute bounds of this Morton cell in 2D
    //     uint64 cellSize = 1u << (MAX_LEVEL - level);
    //     uint64 cellMinX = node.X * cellSize;
    //     uint64 cellMinY = node.Y * cellSize;
    //     uint64 cellMaxX = cellMinX + cellSize - 1;
    //     uint64 cellMaxY = cellMinY + cellSize - 1;
    //
    //     // Check outside
    //     if (cellMaxX < query.MinX || cellMinX > query.MaxX ||
    //         cellMaxY < query.MinY || cellMinY > query.MaxY)
    //     {
    //         return;
    //     }
    //
    //     // Fully inside
    //     if (cellMinX >= query.MinX && cellMaxX <= query.MaxX &&
    //         cellMinY >= query.MinY && cellMaxY <= query.MaxY)
    //     {
    //         outRanges.emplace_back(node.CodeMin, node.CodeMax);
    //         return;
    //     }
    //
    //     // Otherwise, recurse into 4 children (like quadtree)
    //     if (level < MAX_LEVEL)
    //     {
    //         for (int32 child = 0; child < 4; ++child)
    //         {
    //             uint64 childMin = node.CodeMin | (child << (2 * (MAX_LEVEL - level - 1)));
    //             uint64 childMax = childMin | ((1ULL << (2 * (MAX_LEVEL - level - 1))) - 1);
    //             uint64 childX = node.X * 2 + (child & 1);
    //             uint64 childY = node.Y * 2 + ((child >> 1) & 1);
    //             MortonCodeNode childNode = { childX, childY, childMin, childMax };
    //             MortonCodeQuery(query, childNode, outRanges, level + 1);
    //         }
    //     }
    // }

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
