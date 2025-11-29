
#pragma once

#include "Features.h"

namespace Phoenix
{
    struct TestFeature : IFeature
    {
        PHX_FEATURE_BEGIN(TestFeature)
            FEATURE_CHANNEL(Phoenix::FeatureChannels::HandleWorldAction)
        PHX_FEATURE_END()

        bool OnHandleWorldAction(WorldRef world, const FeatureActionArgs& action) override;
    };
}

