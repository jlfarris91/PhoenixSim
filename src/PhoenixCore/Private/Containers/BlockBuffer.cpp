
#include "Containers/BlockBuffer.h"

#include <algorithm>
#include <cstring>

using namespace Phoenix;

BlockBuffer::Block::Block(const BlockDefinition& definition)
    : Definition(definition)
{
}

BlockBuffer::BlockBuffer()
{
}

BlockBuffer::BlockBuffer(const CtorArgs& args)
{
    Blocks.reserve(args.Definitions.size());
    for (const BlockDefinition& definition : args.Definitions)
    {
        Blocks.emplace_back(definition);
    }

    // Sort blocks by priority
    std::ranges::sort(Blocks, [](const Block& a, const Block& b)
    {
        return static_cast<uint8>(a.Definition.Priority) < static_cast<uint8>(b.Definition.Priority);
    });

    uint32_t totalSize = 0;
    for (Block& block : Blocks)
    {
        block.Offset = totalSize;
        totalSize += block.Definition.Size;
    }

    Data = MakeUnique<uint8[]>(totalSize);
    Size = totalSize;

    uint8* dataPtr = Data.get();
    for (Block& block : Blocks)
    {
        block.Definition.Type->PlacementNew(dataPtr + block.Offset);
    }
}

BlockBuffer::BlockBuffer(const BlockBuffer& other)
    : Blocks(other.Blocks)
    , Size(other.Size)
{
    Data = MakeUnique<uint8[]>(other.Size);
    std::memcpy(Data.get(), other.Data.get(), other.Size);
}

BlockBuffer::BlockBuffer(BlockBuffer&& other) noexcept
    : Blocks(MoveTemp(other.Blocks))
    , Data(MoveTemp(other.Data))
    , Size(other.Size)
{
    other.Data = nullptr;
    other.Size = 0;
    other.Blocks.clear();
}

BlockBuffer& BlockBuffer::operator=(const BlockBuffer& other)
{
    if (&other == this)
        return *this;

    if (Size < other.Size)
    {
        Data.release();
        Data = MakeUnique<uint8[]>(other.Size);
        Size = other.Size;
    }

    std::memcpy(Data.get(), other.Data.get(), other.Size);

    return *this;
}

BlockBuffer& BlockBuffer::operator=(BlockBuffer&& other) noexcept
{
    Data = MoveTemp(other.Data);
    Size = other.Size;
    return *this;
}

uint8* BlockBuffer::GetData()
{
    return Data.get();
}

const uint8* BlockBuffer::GetData() const
{
    return Data.get();
}

uint32 BlockBuffer::GetSize() const
{
    return Size;
}

const TArray<BlockBuffer::Block>& BlockBuffer::GetBlocks() const
{
    return Blocks;
}

const BlockBuffer::BlockDefinition* BlockBuffer::GetBlockDefinition(const FName& name) const
{
    for (const Block& block : Blocks)
    {
        if (block.Definition.Name == name)
        {
            return &block.Definition;
        }
    }
    return nullptr;
}

uint8* BlockBuffer::GetBlock(const FName& name)
{
    for (const Block& block : Blocks)
    {
        if (block.Definition.Name == name)
        {
            return Data.get() + block.Offset;
        }
    }
    return nullptr;
}

const uint8* BlockBuffer::GetBlock(const FName& name) const
{
    for (const Block& block : Blocks)
    {
        if (block.Definition.Name == name)
        {
            return Data.get() + block.Offset;
        }
    }
    return nullptr;
}
