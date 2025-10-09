#pragma once

#include <map>

#include "PhoenixSim.h"
#include "Worlds.h"

namespace Phoenix
{
    struct IDebugState;
    struct IDebugRenderer;
    class Session;
}

#define DECLARE_FEATURE(feature) \
    static constexpr FName StaticName = #feature##_n; \
    virtual FName GetName() const override { return StaticName; }

namespace Phoenix
{
    struct Action;
    struct WorldBufferBlockArgs;

    struct PHOENIXSIM_API FeatureUpdateArgs
    {
        simtime_t SimTime = 0;
        dt_t StepHz = 0;
    };

    struct PHOENIXSIM_API FeatureActionArgs
    {
        simtime_t SimTime = 0;
        Action Action;
    };
    
    class PHOENIXSIM_API IFeature : public TSharedAsThis<IFeature>
    {
    public:

        virtual ~IFeature() = default;

        virtual FName GetName() const;
        
        virtual struct FeatureDefinition GetFeatureDefinition();

        virtual void Initialize();
        virtual void Shutdown();

        virtual void OnWorldInitialize(WorldRef world);
        virtual void OnWorldShutdown(WorldRef world);

        virtual void OnPreUpdate(WorldRef world, const FeatureUpdateArgs& args);
        virtual void OnUpdate(WorldRef world, const FeatureUpdateArgs& args);
        virtual void OnPostUpdate(WorldRef world, const FeatureUpdateArgs& args);

        virtual void OnPreHandleAction(WorldRef world, const FeatureActionArgs& action);
        virtual void OnHandleAction(WorldRef world, const FeatureActionArgs& action);
        virtual void OnPostHandleAction(WorldRef world, const FeatureActionArgs& action);

        virtual void OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer);

    protected:

        friend class Session;

        Session* Session = nullptr;
    };

    typedef IFeature* FeaturePtr;
    typedef const IFeature* FeatureConstPtr;
    typedef TSharedPtr<IFeature> FeatureSharedPtr;
    typedef TSharedPtr<const IFeature> FeatureSharedConstPtr;

    enum class EFeatureInsertPosition : uint8
    {
        Default,
        Before,
        After,
    };

    struct PHOENIXSIM_API FeatureInsertPosition
    {
        static const FeatureInsertPosition Default;
        FName FeatureName;
        EFeatureInsertPosition RelativePosition = EFeatureInsertPosition::Default;
    };

    struct PHOENIXSIM_API FeatureChannelInsertArgs
    {
        FeatureChannelInsertArgs() = default;

        FeatureChannelInsertArgs(
            const FName& channelName,
            const FeatureInsertPosition& position = {});

        FName Channel;
        FeatureInsertPosition InsertPosition;
    };

    struct PHOENIXSIM_API FeatureSetCtorArgs
    {
        TArray<FeatureSharedPtr> Features;
    };

    struct FeatureDefinition
    {
        FName Name;
        TArray<WorldBufferBlockArgs> Blocks;
        TArray<FeatureChannelInsertArgs> Channels;
        TArray<FName> DependentFeatures;

        template <class TBlock>
        void RegisterBlock()
        {
            Blocks.emplace_back(TBlock::StaticName, TBlock::StaticType, sizeof(TBlock));
        }

        void RegisterChannel(FName channel, const FeatureInsertPosition& insertPosition = FeatureInsertPosition::Default)
        {
            Channels.emplace_back(channel, insertPosition);
        }
    };

    class PHOENIXSIM_API FeatureSet
    {
    public:

        FeatureSet(const FeatureSetCtorArgs& args);

        FeatureSharedPtr GetFeature(const FName& name) const;

        template <class TFeature>
        TSharedPtr<TFeature> GetFeature(const FName& name) const
        {
            return std::static_pointer_cast<TFeature>(GetFeature(name));
        }

        template <class TFeature>
        TSharedPtr<TFeature> GetFeature() const
        {
            return GetFeature<TFeature>(TFeature::StaticName);
        }

        TArray<FeatureSharedPtr> GetFeatures() const;

        // Gets an array containing all the names of the channels.
        TArray<FName> GetChannelNames() const;
        
        TArray<FeatureSharedPtr> GetChannel(const FName& channelName) const;
        const TArray<FeatureSharedPtr>& GetChannelRef(const FName& channelName) const;

        template <class TCallback>
        void ForEachFeatureInChannel(const FName& channelName, const TCallback& callback)
        {
            auto&& channelEntry = Channels.find(channelName);
            if (channelEntry == Channels.end())
                return;

            const auto& channel = channelEntry->second;
            for (auto&& feature : channel)
            {
                callback(*feature);
            }
        }

    private:
        
        void RegisterFeatureChannels(const TArray<FeatureSharedPtr>& featureDefs);

        static int32 FindChannelInsertIndex(
            const TArray<FeatureSharedPtr>& channelFeatures,
            const FeatureInsertPosition& insertPosition);

        TMap<FName, FeatureSharedPtr> Features;
        TMap<FName, TArray<FeatureSharedPtr>> Channels;
    };
}
