
#include "FeatureTrace.h"

using namespace Phoenix;

FeatureTrace::FeatureTrace()
{
}

void FeatureTrace::PushTrace(WorldRef world, FName name, FName id, ETraceFlags flags, int32 counter)
{
    FeatureTraceScratchBlock& block = world.GetBlockRef<FeatureTraceScratchBlock>();
    if (!block.Events.IsFull())
    {
        block.Events.EmplaceBack(name, id, flags, clock(), counter);
    }
}