
#include "FeatureTrace.h"

using namespace Phoenix;

FeatureTrace::FeatureTrace()
{
    WorldBufferBlockArgs blockArgs;
    blockArgs.Name = FeatureTraceScratchBlock::StaticName;
    blockArgs.Size = sizeof(FeatureTraceScratchBlock);
    blockArgs.BlockType = EWorldBufferBlockType::Scratch;

    FeatureDefinition.Name = StaticName;
    FeatureDefinition.Blocks.push_back(blockArgs);
}

FName FeatureTrace::GetName() const
{
    return StaticName;
}

FeatureDefinition FeatureTrace::GetFeatureDefinition()
{
    return FeatureDefinition;
}

void FeatureTrace::PushTrace(WorldRef world, FName name, FName id, ETraceFlags flags)
{
    FeatureTraceScratchBlock& block = world.GetBlockRef<FeatureTraceScratchBlock>();
    if (!block.Events.IsFull())
    {
        block.Events.PushBack({name, id, flags, clock()});
    }
}