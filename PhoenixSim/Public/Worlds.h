#pragma once

#include "Actions.h"
#include "PhoenixSim.h"

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

#define DECLARE_WORLD_BLOCK(block, type) \
    static constexpr FName StaticName = #block##_n; \
    static constexpr EWorldBufferBlockType StaticType = type;

#define DECLARE_WORLD_BLOCK_STATIC(block) \
    DECLARE_WORLD_BLOCK(block, EWorldBufferBlockType::Static)

#define DECLARE_WORLD_BLOCK_DYNAMIC(block) \
    DECLARE_WORLD_BLOCK(block, EWorldBufferBlockType::Dynamic)

#define DECLARE_WORLD_BLOCK_SCRATCH(block) \
    DECLARE_WORLD_BLOCK(block, EWorldBufferBlockType::Scratch)

namespace Phoenix
{
    enum class PHOENIXSIM_API EWorldBufferBlockType : uint8
    {
        Static,
        Dynamic,
        Scratch
    };

    struct WorldChannels
    {
        static constexpr FName WorldInitialize = "WorldInitialize"_n;
        static constexpr FName WorldShutdown = "WorldShutdown"_n;

        static constexpr FName PreUpdate = "PreUpdate"_n;
        static constexpr FName Update = "Update"_n;
        static constexpr FName PostUpdate = "PostUpdate"_n;

        static constexpr FName PreHandleAction = "PreHandleAction"_n;
        static constexpr FName HandleAction = "HandleAction"_n;
        static constexpr FName PostHandleAction = "PostHandleAction"_n;

        static constexpr FName DebugRender = "DebugRender"_n;
    };

    struct PHOENIXSIM_API WorldBufferBlockArgs
    {
        FName Name;
        EWorldBufferBlockType BlockType = EWorldBufferBlockType::Static;
        uint32 Size = 0;
    };

    struct PHOENIXSIM_API WorldBufferBlock
    {
        WorldBufferBlock() = default;
        WorldBufferBlock(const WorldBufferBlockArgs& args);

        FName Name;
        EWorldBufferBlockType BlockType = EWorldBufferBlockType::Static;
        uint32 Size = 0;
        uint32 Offset = 0;
    };

    struct PHOENIXSIM_API WorldBufferCtorArgs
    {
        TArray<WorldBufferBlockArgs> Blocks;
    };

    class PHOENIXSIM_API WorldBuffer
    {
    public:
        WorldBuffer(const WorldBufferCtorArgs& args);
        WorldBuffer(const WorldBuffer& other);
        WorldBuffer(WorldBuffer&& other) noexcept;
        ~WorldBuffer();

        WorldBuffer& operator=(const WorldBuffer& other);
        WorldBuffer& operator=(WorldBuffer&& other) noexcept;

        uint8* GetData();
        const uint8* GetData() const;

        uint32 GetSize() const;

        const TArray<WorldBufferBlock>& GetBlocks() const;
        
        uint8* GetBlock(const FName& name);
        const uint8* GetBlock(const FName& name) const;

        template <class TBlock>
        TBlock* GetBlock(const FName& name)
        {
            return static_cast<TBlock>(GetBlock<TBlock>(name));
        }

        template <class TBlock>
        const TBlock* GetBlock(const FName& name) const
        {
            return static_cast<TBlock>(GetBlock<TBlock>(name));
        }

    private:
        uint8* Data = nullptr;
        uint32 Size = 0;
        TArray<WorldBufferBlock> Blocks;
    };

    struct PHOENIXSIM_API WorldCtorArgs
    {
        FName Name;
        WorldBufferCtorArgs BufferArgs;
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

        WorldBuffer& GetBuffer();
        const WorldBuffer& GetBuffer() const;

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
            return reinterpret_cast<TBlock*>(GetBlock(TBlock::StaticName));
        }

        template <class TBlock>
        const TBlock* GetBlock() const
        {
            return reinterpret_cast<const TBlock*>(GetBlock(TBlock::StaticName));
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
        WorldBuffer Buffer;
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

    struct PHOENIXSIM_API WorldDynamicBlock
    {
        DECLARE_WORLD_BLOCK_DYNAMIC(WorldDynamicBlock)
        dt_t SimTime = 0;
    };
    
    class PHOENIXSIM_API WorldManager
    {
    public:
        WorldManager(const WorldManagerCtorArgs& args);
        ~WorldManager();

        WorldSharedPtr NewWorld(const FName& name);
        WorldSharedPtr GetWorld(const FName& name) const;

        void Step(const WorldStepArgs& args);

        void SendAction(const WorldSendActionArgs& args);

    private:

        void InitializeWorld(WorldRef world) const;
        void ShutdownWorld(WorldRef world) const;
        void UpdateWorld(WorldRef world, simtime_t time, clock_t stepHz) const;
        void SendActionToWorld(WorldRef world, const Action& action) const;

        TSharedPtr<FeatureSet> FeatureSet;
        TArray<WorldSharedPtr> Worlds;
        WorldBufferCtorArgs WorldBufferCtorArgs;

        PostWorldUpdateDelegate OnPostWorldUpdate;
    };
}

