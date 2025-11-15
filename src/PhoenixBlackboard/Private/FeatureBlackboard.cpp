
#include "FeatureBlackboard.h"

#include "Session.h"

using namespace Phoenix;
using namespace Phoenix::Blackboard;

const SessionBlackboardSet& FeatureBlackboard::GetGlobalBlackboard(SessionConstRef session)
{
    const FeatureBlackboardDynamicSessionBlock& block = session.GetBuffer()->GetBlockRef<FeatureBlackboardDynamicSessionBlock>();
    return block.BlackboardSet;
}

bool FeatureBlackboard::HasGlobalValue(SessionConstRef session, blackboard_key_t key)
{
    const FeatureBlackboardDynamicSessionBlock* block = session.GetBuffer()->GetBlock<FeatureBlackboardDynamicSessionBlock>();
    return block && block->BlackboardSet.HasKey(key);
}

bool FeatureBlackboard::SetGlobalValue(SessionRef session, blackboard_key_t key, blackboard_value_t value)
{
    FeatureBlackboardDynamicSessionBlock* block = session.GetBuffer()->GetBlock<FeatureBlackboardDynamicSessionBlock>();
    return block && block->BlackboardSet.Set(key, value);
}

blackboard_value_t FeatureBlackboard::GetGlobalValue(SessionConstRef session, blackboard_key_t key)
{
    const FeatureBlackboardDynamicSessionBlock* block = session.GetBuffer()->GetBlock<FeatureBlackboardDynamicSessionBlock>();
    return block ? block->BlackboardSet.Get(key) : blackboard_value_t{};
}

bool FeatureBlackboard::TryGetGlobalValue(SessionConstRef session, blackboard_key_t key, blackboard_value_t& outValue)
{
    const FeatureBlackboardDynamicSessionBlock* block = session.GetBuffer()->GetBlock<FeatureBlackboardDynamicSessionBlock>();
    return block && block->BlackboardSet.TryGet(key, outValue);
}

const WorldBlackboardSet& FeatureBlackboard::GetBlackboardSet(WorldConstRef world)
{
    const FeatureBlackboardDynamicWorldBlock& block = world.GetBlockRef<FeatureBlackboardDynamicWorldBlock>();
    return block.BlackboardSet;
}

bool FeatureBlackboard::HasValue(WorldConstRef world, blackboard_key_t key)
{
    const FeatureBlackboardDynamicWorldBlock* block = world.GetBlock<FeatureBlackboardDynamicWorldBlock>();
    return block && block->BlackboardSet.HasKey(key);
}

bool FeatureBlackboard::SetValue(WorldRef world, blackboard_key_t key, blackboard_value_t value)
{
    FeatureBlackboardDynamicWorldBlock* block = world.GetBlock<FeatureBlackboardDynamicWorldBlock>();
    return block && block->BlackboardSet.Set(key, value);
}

blackboard_value_t FeatureBlackboard::GetValue(WorldConstRef world, blackboard_key_t key)
{
    const FeatureBlackboardDynamicWorldBlock* block = world.GetBlock<FeatureBlackboardDynamicWorldBlock>();
    return block ? block->BlackboardSet.Get(key) : blackboard_value_t{};
}

bool FeatureBlackboard::TryGetValue(
    WorldConstRef world,
    blackboard_key_t key,
    blackboard_value_t& outValue)
{
    const FeatureBlackboardDynamicWorldBlock* block = world.GetBlock<FeatureBlackboardDynamicWorldBlock>();
    return block && block->BlackboardSet.TryGet(key, outValue);
}
