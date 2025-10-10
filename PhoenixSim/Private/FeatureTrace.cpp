
#include "FeatureTrace.h"

using namespace Phoenix;

FeatureTrace::FeatureTrace()
{
    FeatureDefinition.Name = StaticName;
    FeatureDefinition.RegisterBlock<FeatureTraceScratchBlock>();
}

FeatureDefinition FeatureTrace::GetFeatureDefinition()
{
    return FeatureDefinition;
}

void FeatureTrace::PushTrace(WorldRef world, FName name, FName id, ETraceFlags flags, int32 counter)
{
    FeatureTraceScratchBlock& block = world.GetBlockRef<FeatureTraceScratchBlock>();
    if (!block.Events.IsFull())
    {
        block.Events.PushBack({name, id, flags, clock(), counter});
    }
}