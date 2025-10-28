
#include "MortonCode.h"

using namespace Phoenix;

namespace PhoenixMortonCodeImpl
{
    void MortonCodeQuery(
        const MortonCodeAABB& query,
        TMortonCodeRangeArray& outRanges,
        int32 cellMinX,
        int32 cellMinY,
        uint32 cellLog)
    {
        uint32 cellSize = 1u << cellLog;
        int32 cellMaxX = cellMinX + cellSize - 1u;
        int32 cellMaxY = cellMinY + cellSize - 1u;

        // Outside?
        if (cellMaxX < query.MinX || cellMinX > query.MaxX ||
            cellMaxY < query.MinY || cellMinY > query.MaxY)
        {
            return;
        }

        // Single cell of size 1, it's overlapping so add its single code.
        if (cellLog == 0)
        {
            uint64 code = ToMortonCode(cellMinX, cellMinY, 0);
            outRanges.emplace_back(code, code);
            return;
        }

        // fully inside?
        if (cellMinX >= query.MinX && cellMaxX <= query.MaxX &&
            cellMinY >= query.MinY && cellMaxY <= query.MaxY)
        {
            // compute Morton base for cell and its range length = 1 << (2*cellLog)
            uint64 baseCode = ToMortonCode(cellMinX, cellMinY, 0);
            uint8 quad = GetMortonCodeQuad(baseCode);
            // clear bottom 2*cellLog bits to align to cell block boundary
            const uint32 lowBits = 2u * cellLog;
            uint64 mask = (lowBits >= 64) ? 0ULL : ((1ULL << lowBits) - 1ULL);
            uint64 blockBase = (baseCode & ~mask) | (int64(quad) << 61);
            uint64 blockMax  = (blockBase | mask) | (int64(quad) << 61);
            outRanges.emplace_back(blockBase, blockMax);
            return;
        }

        // split into 4 children (quadtree)
        uint32 childLog = cellLog - 1;
        uint32 half = cellSize >> 1;

        // children: (0,0), (1,0), (0,1), (1,1) in (dx,dy)
        MortonCodeQuery(query, outRanges, cellMinX,         cellMinY,         childLog);
        MortonCodeQuery(query, outRanges, cellMinX + half,  cellMinY,         childLog);
        MortonCodeQuery(query, outRanges, cellMinX,         cellMinY + half,  childLog);
        MortonCodeQuery(query, outRanges, cellMinX + half,  cellMinY + half,  childLog);
    }
}

MortonCodeAABB Phoenix::ToMortonCodeAABB(Vec2 pos, Distance radius)
{
    int32 lox = (int32)(pos.X - radius);
    int32 hix = (int32)(pos.X + radius);
    int32 loy = (int32)(pos.Y - radius);
    int32 hiy = (int32)(pos.Y + radius);

    MortonCodeAABB aabb;
    aabb.MinX = ScaleToMortonCode(lox);
    aabb.MinY = ScaleToMortonCode(loy);
    aabb.MaxX = ScaleToMortonCode(hix);
    aabb.MaxY = ScaleToMortonCode(hiy);

    if (lox < 0) --aabb.MinX;
    if (loy < 0) --aabb.MinY;
    if (hix < 0) --aabb.MaxX;
    if (hiy < 0) --aabb.MaxY;

    return aabb;
}

void Phoenix::MortonCodeQuery(const MortonCodeAABB& query, TMortonCodeRangeArray& outRanges, uint32 gridBits)
{
    if (gridBits == 0 || gridBits > 32) return;

    constexpr auto b = (sizeof(Distance::ValueT) << 3) - Distance::B;

    // start at root: cell covering [-2^(B - 1), 2^B - 1]
    constexpr int32 cellMinX = -(1 << (b - 1));
    constexpr int32 cellMinY = -(1 << (b - 1));

    PhoenixMortonCodeImpl::MortonCodeQuery(query, outRanges, cellMinX, cellMinY, b);
}
