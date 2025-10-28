#pragma once

#include "Actions.h"
#include "BlockBuffer.h"

namespace Phoenix
{
    struct IDebugRenderer;
}

namespace Phoenix
{
    struct IDebugState;
}

namespace Phoenix
{
    class FeatureSet;
    class FeatureSet;
}

namespace Phoenix
{
    struct PHOENIXSIM_API WorldCtorArgs
    {
        FName Name;
        BlockBuffer::CtorArgs Blocks;
    };

    class PHOENIXSIM_API World
    {
    public:

        World(const WorldCtorArgs& args);
        World(const World& other);
        World(World&& other) noexcept;
        ~World() = default;

        FName GetName() const;

        World& operator=(const World& other);
        World& operator=(World&& other) noexcept;

        BlockBuffer& GetBuffer();
        const BlockBuffer& GetBuffer() const;

        uint8* GetBlock(const FName& name);
        const uint8* GetBlock(const FName& name) const;

        template <class TBlock>
        TBlock* GetBlock(const FName& name)
        {
            return reinterpret_cast<TBlock*>(GetBlock(name));
        }

        template <class TBlock>
        const TBlock* GetBlock(const FName& name) const
        {
            return reinterpret_cast<const TBlock*>(GetBlock(name));
        }

        template <class TBlock>
        TBlock* GetBlock()
        {
            return reinterpret_cast<TBlock*>(GetBlock(TBlock::StaticTypeName));
        }

        template <class TBlock>
        const TBlock* GetBlock() const
        {
            return reinterpret_cast<const TBlock*>(GetBlock(TBlock::StaticTypeName));
        }

        template <class TBlock>
        TBlock& GetBlockRef()
        {
            return *GetBlock<TBlock>();
        }

        template <class TBlock>
        const TBlock& GetBlockRef() const
        {
            return *GetBlock<TBlock>();
        }

    private:
        FName Name;
        BlockBuffer Buffer;
    };

    typedef World* WorldPtr;
    typedef const World* WorldConstPtr;

    typedef World& WorldRef;
    typedef const World& WorldConstRef;

    typedef TSharedPtr<World> WorldSharedPtr;
    
    using PostWorldUpdateDelegate = TDelegate<void(WorldConstRef world)>;

    struct PHOENIXSIM_API WorldManagerCtorArgs
    {
        TSharedPtr<FeatureSet> FeatureSet;
        PostWorldUpdateDelegate OnPostWorldUpdate;
    };

    struct PHOENIXSIM_API WorldStepArgs
    {
        simtime_t SimTime = 0;
        clock_t StepHz = 0;
        FName WorldName = FName::None;
    };

    struct PHOENIXSIM_API WorldSendActionArgs
    {
        Action Action;
        FName WorldName = FName::None;
    };

    struct PHOENIXSIM_API WorldDynamicBlock : BufferBlockBase
    {
        PHX_DECLARE_BLOCK_DYNAMIC(WorldDynamicBlock)
        dt_t SimTime = 0;
    };

    class PHOENIXSIM_API WorldManager
    {
    public:
        WorldManager(const WorldManagerCtorArgs& args);
        ~WorldManager();

        WorldSharedPtr NewWorld(const FName& name);
        WorldSharedPtr GetWorld(const FName& name) const;

        WorldSharedPtr GetPrimaryWorld() const;

        void Step(const WorldStepArgs& args);

        void SendAction(const WorldSendActionArgs& args);

    private:

        void InitializeWorld(WorldRef world) const;
        void ShutdownWorld(WorldRef world) const;
        void UpdateWorld(WorldRef world, simtime_t time, clock_t stepHz) const;
        void SendActionToWorld(WorldRef world, const Action& action) const;

        TSharedPtr<FeatureSet> FeatureSet;
        TArray<WorldSharedPtr> Worlds;
        BlockBuffer::CtorArgs WorldBufferBlockArgs;

        PostWorldUpdateDelegate OnPostWorldUpdate;
    };
}

