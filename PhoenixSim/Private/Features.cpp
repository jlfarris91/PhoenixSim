#include "Features.h"

using namespace Phoenix;

FName IFeature::GetName() const
{
    return FName::None;
}

FeatureDefinition IFeature::GetFeatureDefinition()
{
    return {};
}

void IFeature::Initialize()
{
}

void IFeature::Shutdown()
{
}

void IFeature::OnWorldInitialize(WorldRef world)
{
}

void IFeature::OnWorldShutdown(WorldRef world)
{
}

void IFeature::OnPreUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
}

void IFeature::OnUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
}

void IFeature::OnPostUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
}

void IFeature::OnPreHandleAction(WorldRef world, const FeatureActionArgs& args)
{
}

void IFeature::OnHandleAction(WorldRef world, const FeatureActionArgs& args)
{
}

void IFeature::OnPostHandleAction(WorldRef world, const FeatureActionArgs& args)
{
}

FeatureSharedPtr FeatureSet::GetFeature(const FName& name) const
{
    auto&& feature = Features.find(name);
    if (feature == Features.end())
        return nullptr;
    return feature->second;
}

TArray<FeatureSharedPtr> FeatureSet::GetFeatures() const
{
    TArray<FeatureSharedPtr> features;
    features.reserve(Features.size());
    for (auto&& feature : Features)
    {
        features.push_back(feature.second);
    }
    return features;
}

const FeatureInsertPosition FeatureInsertPosition::Default = {};

FeatureChannelInsertArgs::FeatureChannelInsertArgs(
    const FName& channelName,
    const FeatureInsertPosition& position)
    : Channel(channelName)
    , InsertPosition(position)
{
    
}

FeatureSet::FeatureSet(const FeatureSetCtorArgs& args)
{
    for (const FeatureSharedPtr& feature : args.Features)
    {
        Features.insert_or_assign(feature->GetName(), feature);
    }

    RegisterFeatureChannels(args.Features);
}

TArray<FName> FeatureSet::GetChannelNames() const
{
    TArray<FName> channelNames;
    channelNames.reserve(Channels.size());
    for (auto&& channels : Channels)
    {
        channelNames.push_back(channels.first);
    }
    return channelNames;
}

TArray<FeatureSharedPtr> FeatureSet::GetChannel(const FName& channelName) const
{
    TArray<FeatureSharedPtr> features;
    auto&& channelIter = Channels.find(channelName);
    if (channelIter != Channels.end())
    {
        features = channelIter->second;
    }
    return features;
}

const TArray<FeatureSharedPtr>& FeatureSet::GetChannelRef(const FName& channelName) const
{
    auto channelIter = Channels.find(channelName);
    if (channelIter == Channels.end())
    {
        static TArray<FeatureSharedPtr> staticEmpty;
        return staticEmpty;
    }
    return channelIter->second;
}

struct FeatureChannelInsert
{
    FName Feature;
    FName Channel;
    FeatureInsertPosition InsertPosition;
};

void FeatureSet::RegisterFeatureChannels(const TArray<FeatureSharedPtr>& features)
{
    TArray<FeatureChannelInsert> remainingInserts;

    for (const FeatureSharedPtr& feature : features)
    {
        FeatureDefinition featureDefinition = feature->GetFeatureDefinition();
        for (const FeatureChannelInsertArgs& channelInsert : featureDefinition.Channels)
        {
            remainingInserts.emplace_back(feature->GetName(), channelInsert.Channel, channelInsert.InsertPosition);
        }
    }

    while (!remainingInserts.empty())
    {
        bool insertedAnything = false;

        for (const FeatureChannelInsert& featureInsert : remainingInserts)
        {
            auto&& featureIter = Features.find(featureInsert.Feature);
            if (featureIter == Features.end())
            {
                // TODO (jfarris): report error here
                continue;
            }
            
            auto&& channelIter = Channels.find(featureInsert.Channel);

            // The channel doesn't exist yet
            if (channelIter == Channels.end())
            {
                // Only create the channel if this is the first default feature
                // Further iterations will add features with relative positions
                if (featureInsert.InsertPosition.RelativePosition == EFeatureInsertPosition::Default)
                {
                    Channels[featureInsert.Channel] = { featureIter->second };
                    remainingInserts.erase(remainingInserts.begin());
                    insertedAnything = true;
                }
                break;
            }

            // The channel exists, try to insert the feature relative to another existing in the channel
            TArray<FeatureSharedPtr>& channel = Channels[featureInsert.Channel];

            int32 insertIndex = FindChannelInsertIndex(channel, featureInsert.InsertPosition);

            // The relative feature doesn't exist in the channel yet, try again later.
            if (insertIndex == INDEX_NONE)
            {
                continue;
            }

            channel.insert(channel.begin() + insertIndex, featureIter->second);
            remainingInserts.erase(remainingInserts.begin());
            insertedAnything = true;
        }

        if (!insertedAnything)
        {
            break;
        }
    }

    // TODO (jfarris): error! couldn't insert a feature
    assert(remainingInserts.empty());
}

int32 FeatureSet::FindChannelInsertIndex(
    const TArray<FeatureSharedPtr>& channelFeatures,
    const FeatureInsertPosition& insertPosition)
{
    if (insertPosition.RelativePosition == EFeatureInsertPosition::Default)
    {
        return static_cast<int32>(channelFeatures.size());
    }

    for (size_t i = 0; i < channelFeatures.size(); ++i)
    {
        if (channelFeatures[i]->GetName() != insertPosition.FeatureName)
        {
            continue;
        }

        if (insertPosition.RelativePosition == EFeatureInsertPosition::Before)
        {
            return static_cast<int32>(i);
        }

        if (insertPosition.RelativePosition == EFeatureInsertPosition::After)
        {
            return static_cast<int32>(i) + 1;
        }
    }

    return INDEX_NONE;
}
