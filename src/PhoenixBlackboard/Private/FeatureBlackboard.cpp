
#include "FeatureBlackboard.h"

#include "Session.h"

using namespace Phoenix;
using namespace Phoenix::Blackboard;

void FeatureBlackboard::OnPostUpdate(const FeatureUpdateArgs& args)
{
    IFeature::OnPostUpdate(args);
    
    FeatureBlackboardDynamicSessionBlock& block = Session->GetBuffer()->GetBlockRef<FeatureBlackboardDynamicSessionBlock>();
    block.Blackboard.SortAndCompact();
}

void FeatureBlackboard::OnPostWorldUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    IFeature::OnPostWorldUpdate(world, args);

    FeatureBlackboardDynamicWorldBlock& block = world.GetBlockRef<FeatureBlackboardDynamicWorldBlock>();
    block.Blackboard.SortAndCompact();
}

SessionBlackboard& FeatureBlackboard::GetGlobalBlackboard(SessionRef session)
{
    FeatureBlackboardDynamicSessionBlock& block = session.GetBuffer()->GetBlockRef<FeatureBlackboardDynamicSessionBlock>();
    return block.Blackboard;
}

const SessionBlackboard& FeatureBlackboard::GetGlobalBlackboard(SessionConstRef session)
{
    const FeatureBlackboardDynamicSessionBlock& block = session.GetBuffer()->GetBlockRef<FeatureBlackboardDynamicSessionBlock>();
    return block.Blackboard;
}

WorldBlackboard& FeatureBlackboard::GetBlackboard(WorldRef world)
{
    FeatureBlackboardDynamicWorldBlock& block = world.GetBlockRef<FeatureBlackboardDynamicWorldBlock>();
    return block.Blackboard;
}

const WorldBlackboard& FeatureBlackboard::GetBlackboard(WorldConstRef world)
{
    const FeatureBlackboardDynamicWorldBlock& block = world.GetBlockRef<FeatureBlackboardDynamicWorldBlock>();
    return block.Blackboard;
}
