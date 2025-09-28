
#include "MortonCode.h"

using namespace Phoenix;

namespace PhoenixMortonCodeImpl
{
    void MortonCodeQuery(
        const MortonCodeAABB& query,
        TMortonCodeRangeArray& outRanges,
        uint32 cellMinX,
        uint32 cellMinY,
        uint32 cellLog)
    {
        uint32 cellSize = 1u << cellLog;
        uint32 cellMaxX = cellMinX + cellSize - 1u;
        uint32 cellMaxY = cellMinY + cellSize - 1u;

        // outside?
        if (cellMaxX < query.MinX || cellMinX > query.MaxX ||
            cellMaxY < query.MinY || cellMinY > query.MaxY)
        {
            return;
        }

        // fully inside?
        if (cellMinX >= query.MinX && cellMaxX <= query.MaxX &&
            cellMinY >= query.MinY && cellMaxY <= query.MaxY)
        {
            // compute Morton base for cell and its range length = 1 << (2*cellLog)
            uint64 baseCode = MortonCode(cellMinX, cellMinY);
            // clear bottom 2*cellLog bits to align to cell block boundary
            const uint32 lowBits = 2u * cellLog;
            uint64 mask = (lowBits >= 64) ? 0ULL : ((1ULL << lowBits) - 1ULL);
            uint64 blockBase = baseCode & ~mask;
            uint64 blockMax  = blockBase | mask;
            outRanges.emplace_back(blockBase, blockMax);
            return;
        }

        // otherwise partially overlap: split unless at leaf (cellLog == 0)
        if (cellLog == 0)
        {
            // single cell of size 1 -- it's overlapping so add its single code
            uint64 code = MortonCode(cellMinX, cellMinY);
            outRanges.emplace_back(code, code);
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
    uint32 lox = (uint32)(pos.X - radius);
    uint32 hix = (uint32)(pos.X + radius);
    uint32 loy = (uint32)(pos.Y - radius);
    uint32 hiy = (uint32)(pos.Y + radius);

    MortonCodeAABB aabb;
    aabb.MinX = lox >> MortonCodeGridBits;
    aabb.MinY = loy >> MortonCodeGridBits;
    aabb.MaxX = hix >> MortonCodeGridBits;
    aabb.MaxY = hiy >> MortonCodeGridBits;

    return aabb;
}

void Phoenix::MortonCodeQuery(const MortonCodeAABB& query, TMortonCodeRangeArray& outRanges, uint32 gridBits)
{
    if (gridBits == 0 || gridBits > 32) return;

    // start at root: cell covering [0, 2^B - 1]
    PhoenixMortonCodeImpl::MortonCodeQuery(query, outRanges, 0u, 0u, gridBits);
}
