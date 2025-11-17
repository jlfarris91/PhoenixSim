
#include "FeatureSteering.h"

#include "FeatureECS.h"

using namespace Phoenix;
using namespace Phoenix::Steering;

void FeatureSteering::Initialize()
{
    IFeature::Initialize();
    
    SteeringSystem = MakeShared<Steering::SteeringSystem>();

    TSharedPtr<ECS::FeatureECS> featureECS = Session->GetFeatureSet()->GetFeature<ECS::FeatureECS>();
    featureECS->RegisterSystem(SteeringSystem);
}

bool FeatureSteering::OnHandleWorldAction(WorldRef world, const FeatureActionArgs& action)
{
    return IFeature::OnHandleWorldAction(world, action);
}
