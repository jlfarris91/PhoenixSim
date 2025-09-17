
#include "Worlds.h"

#include <algorithm>
#include <memory>

#include "Features.h"


using namespace Phoenix;

WorldBufferBlock::WorldBufferBlock(const WorldBufferBlockArgs& args)
    : Name(args.Name)
    , BlockType(args.BlockType)
    , Size(args.Size)
{
}

WorldBuffer::WorldBuffer(const WorldBufferCtorArgs& args)
{
    Blocks.reserve(args.Blocks.size());
    for (const WorldBufferBlockArgs& blockArgs : args.Blocks)
    {
        Blocks.emplace_back(blockArgs);
    }

    std::sort(Blocks.begin(), Blocks.end(), [](const WorldBufferBlock& a, const WorldBufferBlock& b)
    {
        return static_cast<uint8_t>(a.BlockType) < static_cast<uint8_t>(b.BlockType);
    });

    uint32_t totalSize = 0;
    for (WorldBufferBlock& block : Blocks)
    {
        block.Offset = totalSize;
        totalSize += block.Size;
    }

    Data = new uint8_t[totalSize];
    Size = totalSize;

    memset(Data, 0, totalSize);
}

WorldBuffer::WorldBuffer(const WorldBuffer& other)
    : Size(other.Size)
    , Blocks(other.Blocks)
{
    Data = new uint8_t[Size];
    std::memcpy(Data, other.Data, other.Size);
}

WorldBuffer::WorldBuffer(WorldBuffer&& other) noexcept
    : Data(other.Data)
    , Size(other.Size)
    , Blocks(std::move(other.Blocks))
{
    other.Data = nullptr;
    other.Size = 0;
    other.Blocks.clear();
}

WorldBuffer::~WorldBuffer()
{
    delete Data;
    Data = nullptr;
}

WorldBuffer& WorldBuffer::operator=(const WorldBuffer& other)
{
    if (&other == this)
        return *this;

    delete Data;
    Data = new uint8_t[other.Size];
    Size = other.Size;
    std::memcpy(Data, other.Data, other.Size);

    return *this;
}

WorldBuffer& WorldBuffer::operator=(WorldBuffer&& other) noexcept
{
    Data = other.Data;
    Size = other.Size;
    return *this;
}

uint8_t* WorldBuffer::GetData()
{
    return Data;
}

const uint8_t* WorldBuffer::GetData() const
{
    return Data;
}

uint32_t WorldBuffer::GetSize() const
{
    return Size;
}

const TArray<WorldBufferBlock>& WorldBuffer::GetBlocks() const
{
    return Blocks;
}

uint8_t* WorldBuffer::GetBlock(const FName& name)
{
    for (const WorldBufferBlock& block : Blocks)
    {
        if (block.Name == name)
        {
            return Data + block.Offset;
        }
    }
    return nullptr;
}

const uint8_t* WorldBuffer::GetBlock(const FName& name) const
{
    for (const WorldBufferBlock& block : Blocks)
    {
        if (block.Name == name)
        {
            return Data + block.Offset;
        }
    }
    return nullptr;
}

World::World(const WorldCtorArgs& args)
    : Name(args.Name)
    , Buffer(args.BufferArgs)
{
}

World::World(const World& other)
    : Name(other.Name)
    , Buffer(other.Buffer)
{
}

World::World(World&& other) noexcept
    : Name(other.Name)
    , Buffer(std::move(other.Buffer))
{
}

FName World::GetName() const
{
    return Name;
}

World& World::operator=(const World& other)
{
    Buffer = other.Buffer;
    return *this;
}

World& World::operator=(World&& other) noexcept
{
    Buffer = std::move(other.Buffer);
    return *this;
}

WorldBuffer& World::GetBuffer()
{
    return Buffer;
}

const WorldBuffer& World::GetBuffer() const
{
    return Buffer;
}

uint8_t* World::GetBlock(const FName& name)
{
    return Buffer.GetBlock(name);
}

const uint8_t* World::GetBlock(const FName& name) const
{
    return Buffer.GetBlock(name);
}

WorldManager::WorldManager(const WorldManagerCtorArgs& args)
    : FeatureSet(args.FeatureSet)
    , OnPostWorldUpdate(args.OnPostWorldUpdate)
{
    for (const FeatureSharedPtr& feature : FeatureSet->GetFeatures())
    {
        FeatureDefinition worldFeatureDef = feature->GetFeatureDefinition();
        for (WorldBufferBlockArgs& blockArgs : worldFeatureDef.Blocks)
        {
            WorldBufferCtorArgs.Blocks.push_back(blockArgs);
        }
    }
}

WorldManager::~WorldManager()
{
}

WorldSharedPtr WorldManager::NewWorld(const FName& name)
{
    WorldSharedPtr world = GetWorld(name);
    if (world)
    {
        // TODO (jfarris): world with this name already exists!
        return world;
    }

    WorldCtorArgs worldCtorArgs;
    worldCtorArgs.Name = name;
    worldCtorArgs.BufferArgs = WorldBufferCtorArgs;

    world = std::make_shared<World>(worldCtorArgs);
    Worlds.push_back(world);

    return world;
}

WorldSharedPtr WorldManager::GetWorld(const FName& name) const
{
    for (const WorldSharedPtr& world : Worlds)
    {
        if (world->GetName() == name)
            return world;
    }
    return nullptr;
}

void WorldManager::Step(const WorldStepArgs& args)
{
    TArray<WorldSharedPtr> worlds;

    if (args.WorldName != FName::None)
    {
        if (WorldSharedPtr world = GetWorld(args.WorldName))
        {
            worlds.push_back(world);
        }
    }
    else
    {
        worlds = Worlds;
    }

    // TODO (jfarris): parallelize
    for (const WorldSharedPtr& world : worlds)
    {
        UpdateWorld(*world, args.SimTime);
    }
}

void WorldManager::SendAction(const WorldSendActionArgs& args)
{
    TArray<WorldSharedPtr> worlds;

    if (args.WorldName != FName::None)
    {
        if (WorldSharedPtr world = GetWorld(args.WorldName))
        {
            worlds.push_back(world);
        }
    }
    else
    {
        worlds = Worlds;
    }

    // TODO (jfarris): parallelize
    for (const WorldSharedPtr& world : worlds)
    {
        SendActionToWorld(*world, args.Action);
    }
}

void WorldManager::InitializeWorld(WorldRef world) const
{
    TArray<FeatureSharedPtr> channelFeatures = FeatureSet->GetChannelRef(WorldChannels::WorldInitialize);
    for (const FeatureSharedPtr& feature : channelFeatures)
    {
        feature->OnWorldInitialize(world);
    }
}

void WorldManager::ShutdownWorld(WorldRef world) const
{
    TArray<FeatureSharedPtr> channelFeatures = FeatureSet->GetChannelRef(WorldChannels::WorldShutdown);
    for (const FeatureSharedPtr& feature : channelFeatures)
    {
        feature->OnWorldShutdown(world);
    }
}

void WorldManager::UpdateWorld(WorldRef world, simtime_t time) const
{
    FeatureUpdateArgs updateArgs;
    updateArgs.SimTime = time;
    
    // Pre-update
    {
        const TArray<FeatureSharedPtr>& channelFeatures = FeatureSet->GetChannelRef(WorldChannels::PreUpdate);
        for (const FeatureSharedPtr& feature : channelFeatures)
        {
            feature->OnPreUpdate(world, updateArgs);
        }
    }

    // Update
    {
        const TArray<FeatureSharedPtr>& channelFeatures = FeatureSet->GetChannelRef(WorldChannels::Update);
        for (const FeatureSharedPtr& feature : channelFeatures)
        {
            feature->OnUpdate(world, updateArgs);
        }
    }

    // Post-update
    {
        const TArray<FeatureSharedPtr>& channelFeatures = FeatureSet->GetChannelRef(WorldChannels::PostUpdate);
        for (const FeatureSharedPtr& feature : channelFeatures)
        {
            feature->OnPostUpdate(world, updateArgs);
        }
    }

    OnPostWorldUpdate(world);
}

void WorldManager::SendActionToWorld(WorldRef world, const Action& action) const
{
    FeatureActionArgs actionArgs;
    actionArgs.Action = action;
    
    // Pre handle action
    {
        const TArray<FeatureSharedPtr>& channelFeatures = FeatureSet->GetChannelRef(WorldChannels::PreHandleAction);
        for (const FeatureSharedPtr& feature : channelFeatures)
        {
            feature->OnPreHandleAction(world, actionArgs);
        }
    }

    // Handle action
    {
        const TArray<FeatureSharedPtr>& channelFeatures = FeatureSet->GetChannelRef(WorldChannels::HandleAction);
        for (const FeatureSharedPtr& feature : channelFeatures)
        {
            feature->OnHandleAction(world, actionArgs);
        }
    }

    // Post handle action
    {
        const TArray<FeatureSharedPtr>& channelFeatures = FeatureSet->GetChannelRef(WorldChannels::PostHandleAction);
        for (const FeatureSharedPtr& feature : channelFeatures)
        {
            feature->OnPostHandleAction(world, actionArgs);
        }
    }
}
