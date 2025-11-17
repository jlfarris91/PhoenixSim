
#pragma once

#include "DLLExport.h"
#include "Features.h"
#include "SteeringSystem.h"

namespace Phoenix::Steering
{
    struct FeatureSteeringDynamicBlock : BufferBlockBase
    {
        PHX_DECLARE_BLOCK_DYNAMIC(FeatureSteeringDynamicBlock)
    };

    class PHOENIX_STEERING_API FeatureSteering : public IFeature
    {
        PHX_FEATURE_BEGIN(FeatureSteering)
            FEATURE_WORLD_BLOCK(FeatureSteeringDynamicBlock)
            FEATURE_CHANNEL(FeatureChannels::HandleWorldAction)
        PHX_FEATURE_END()

    public:

        void Initialize() override;

        bool OnHandleWorldAction(WorldRef world, const FeatureActionArgs& action) override;

        TSharedPtr<SteeringSystem> SteeringSystem;
    };
}
