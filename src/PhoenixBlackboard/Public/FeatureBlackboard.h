
#pragma once

#include "FixedBlackboard.h"
#include "DLLExport.h"
#include "Features.h"
#include "Session.h"

#ifndef PHX_BLACKBOARD_MAX_GLOBAL_SIZE
#define PHX_BLACKBOARD_MAX_GLOBAL_SIZE 8192
#endif

#ifndef PHX_BLACKBOARD_MAX_WORLD_SIZE
#define PHX_BLACKBOARD_MAX_WORLD_SIZE (16384 * 8)
#endif

namespace Phoenix::Blackboard
{
    using SessionBlackboard = TFixedBlackboard<PHX_BLACKBOARD_MAX_GLOBAL_SIZE>;
    using WorldBlackboard = TFixedBlackboard<PHX_BLACKBOARD_MAX_WORLD_SIZE>;
    
    struct FeatureBlackboardDynamicSessionBlock : BufferBlockBase
    {
        PHX_DECLARE_BLOCK_DYNAMIC(FeatureBlackboardDynamicSessionBlock)
        SessionBlackboard Blackboard;
    };

    struct FeatureBlackboardDynamicWorldBlock : BufferBlockBase
    {
        PHX_DECLARE_BLOCK_DYNAMIC(FeatureBlackboardDynamicWorldBlock)
        WorldBlackboard Blackboard;
    };
    
    class PHOENIX_BLACKBOARD_API FeatureBlackboard final : public IFeature
    {
        PHX_FEATURE_BEGIN(FeatureBlackboard)
            FEATURE_SESSION_BLOCK(FeatureBlackboardDynamicSessionBlock)
            FEATURE_WORLD_BLOCK(FeatureBlackboardDynamicWorldBlock)
            FEATURE_CHANNEL(FeatureChannels::PostWorldUpdate)
        PHX_FEATURE_END()

    public:

        void OnPostUpdate(const FeatureUpdateArgs& args) override;
        void OnPostWorldUpdate(WorldRef world, const FeatureUpdateArgs& args) override;

        //
        // Session-level blackboard
        //

        static SessionBlackboard& GetGlobalBlackboard(SessionRef session);
        static const SessionBlackboard& GetGlobalBlackboard(SessionConstRef session);

        //
        // World-level blackboard
        //

        static WorldBlackboard& GetBlackboard(WorldRef world);
        static const WorldBlackboard& GetBlackboard(WorldConstRef world);
    };
}
