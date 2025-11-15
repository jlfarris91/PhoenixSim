
#pragma once

#include "FixedBlackboardSet.h"
#include "DLLExport.h"
#include "Features.h"
#include "Session.h"

#ifndef PHX_BLACKBOARD_MAX_GLOBAL_SIZE
#define PHX_BLACKBOARD_MAX_GLOBAL_SIZE 1024
#endif

#ifndef PHX_BLACKBOARD_MAX_WORLD_SIZE
#define PHX_BLACKBOARD_MAX_WORLD_SIZE 16384
#endif

namespace Phoenix
{
    namespace Blackboard
    {
        using SessionBlackboardSet = TFixedBlackboardSet<PHX_BLACKBOARD_MAX_GLOBAL_SIZE>;
        using WorldBlackboardSet = TFixedBlackboardSet<PHX_BLACKBOARD_MAX_WORLD_SIZE>;
        
        struct FeatureBlackboardDynamicSessionBlock : BufferBlockBase
        {
            PHX_DECLARE_BLOCK_DYNAMIC(FeatureBlackboardDynamicSessionBlock)
            SessionBlackboardSet BlackboardSet;
        };

        struct FeatureBlackboardDynamicWorldBlock : BufferBlockBase
        {
            PHX_DECLARE_BLOCK_DYNAMIC(FeatureBlackboardDynamicWorldBlock)
            WorldBlackboardSet BlackboardSet;
        };
        
        class PHOENIX_BLACKBOARD_API FeatureBlackboard final : public IFeature
        {
            PHX_FEATURE_BEGIN(FeatureBlackboard)
                FEATURE_SESSION_BLOCK(FeatureBlackboardDynamicSessionBlock)
                FEATURE_WORLD_BLOCK(FeatureBlackboardDynamicWorldBlock)
            PHX_FEATURE_END()

        public:

            //
            // Session-level blackboard
            //

            static const SessionBlackboardSet& GetGlobalBlackboard(SessionConstRef session);

            static bool HasGlobalValue(SessionConstRef session, blackboard_key_t key);

            static bool SetGlobalValue(SessionRef session, blackboard_key_t key, blackboard_value_t value);

            template <class T>
            bool SetGlobalValue(SessionRef session, blackboard_key_t key, blackboard_value_t value)
            {
                return SetGlobalValue(session, key, BlackboardKeyConverter<T>::ConvertTo(value));
            }

            static blackboard_value_t GetGlobalValue(SessionConstRef session, blackboard_key_t key);

            template <class T>
            static T GetGlobalValue(SessionConstRef session, blackboard_key_t key)
            {
                return BlackboardKeyConverter<T>::ConvertFrom(GetGlobalValue(session, key));
            }

            static bool TryGetGlobalValue(SessionConstRef session, blackboard_key_t key, blackboard_value_t& outValue);

            template <class T>
            static bool TryGetGlobalValue(SessionConstRef session, blackboard_key_t key, T& outValue)
            {
                blackboard_value_t value;
                if (!TryGetGlobalValue(session, key, value))
                {
                    return false;
                }
                outValue = BlackboardKeyConverter<T>::ConvertFrom(value);
                return true;
            }

            //
            // World-level blackboard
            //

            static const WorldBlackboardSet& GetBlackboardSet(WorldConstRef world);

            static bool HasValue(WorldConstRef world, blackboard_key_t key);

            static bool SetValue(WorldRef world, blackboard_key_t key, blackboard_value_t value);

            template <class T>
            bool SetValue(WorldRef world, blackboard_key_t key, blackboard_value_t value)
            {
                return SetValue(world, key, BlackboardKeyConverter<T>::ConvertTo(value));
            }

            static blackboard_value_t GetValue(WorldConstRef world, blackboard_key_t key);

            template <class T>
            static T GetValue(WorldConstRef world, blackboard_key_t key)
            {
                return BlackboardKeyConverter<T>::ConvertFrom(GetValue(world, key));
            }

            static bool TryGetValue(WorldConstRef world, blackboard_key_t key, blackboard_value_t& outValue);

            template <class T>
            static bool TryGetValue(WorldConstRef world, blackboard_key_t key, T& outValue)
            {
                blackboard_value_t value;
                if (!TryGetValue(world, key, value))
                {
                    return false;
                }
                outValue = BlackboardKeyConverter<T>::ConvertFrom(value);
                return true;
            }
        };
    }
}
