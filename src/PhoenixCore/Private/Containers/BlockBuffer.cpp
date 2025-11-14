
#include "Containers/BlockBuffer.h"

#include <algorithm>
#include <cstring>

#include "Profiling.h"

using namespace Phoenix;

BlockBuffer::Block::Block(const BlockDefinition& definition)
    : Definition(definition)
{
}

BlockBuffer::BlockBuffer()
= default;

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

    size_t totalSize = 0;
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
        block.Definition.Type->DefaultConstruct(dataPtr + block.Offset);
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
    PHX_PROFILE_ZONE_SCOPED_N("BlockBufferCopy");

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

size_t BlockBuffer::GetSize() const
{
    return Size;
}

const TArray<BlockBuffer::Block>& BlockBuffer::GetBlocks() const
{
    return Blocks;
}

const BlockBuffer::BlockDefinition* BlockBuffer::GetBlockDefinition(const FName& name) const
{
    PHX_PROFILE_ZONE_SCOPED;

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
    PHX_PROFILE_ZONE_SCOPED;

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
    PHX_PROFILE_ZONE_SCOPED;

    for (const Block& block : Blocks)
    {
        if (block.Definition.Name == name)
        {
            return Data.get() + block.Offset;
        }
    }
    return nullptr;
}
