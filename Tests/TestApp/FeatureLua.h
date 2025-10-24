
#pragma once

#include "Features.h"

#include <sol/sol.hpp>

namespace Phoenix
{
    struct FeatureLuaDynamicBlock : BufferBlockBase
    {
        PHX_DECLARE_BLOCK_DYNAMIC(FeatureLuaDynamicBlock)

        sol::state State;
    };

    class FeatureLua : public IFeature
    {
    public:

        PHX_FEATURE_BEGIN(FeatureLua)
            FEATURE_SESSION_BLOCK(FeatureLuaDynamicBlock)
        PHX_FEATURE_END()

        void Initialize() override;
        void Shutdown() override;

        void OnUpdate(const FeatureUpdateArgs& args) override;
    };
}
