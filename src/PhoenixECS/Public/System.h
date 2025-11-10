
#pragma once

#include "DLLExport.h"
#include "Reflection.h"
#include "Worlds.h"

namespace Phoenix
{
    class Session;

    namespace ECS
    {
        struct PHOENIXECS_API SystemUpdateArgs
        {
            simtime_t SimTime = 0;
            DeltaTime DeltaTime;
        };

        struct PHOENIXECS_API SystemActionArgs
        {
            simtime_t SimTime = 0;
            Action Action;
        };

        class PHOENIXECS_API ISystem
        {
            PHX_DECLARE_INTERFACE(ISystem)

            virtual FName GetName() const { return FName::None; }

            virtual void OnPreUpdate(const SystemUpdateArgs& args) {}
            virtual void OnUpdate(const SystemUpdateArgs& args) {}
            virtual void OnPostUpdate(const SystemUpdateArgs& args) {}

            virtual bool OnPreHandleAction(const SystemActionArgs& args) { return false; }
            virtual bool OnHandleAction(const SystemActionArgs& args) { return false; }
            virtual bool OnPostHandleAction(const SystemActionArgs& args) { return false; }

            virtual void OnWorldInitialize(WorldRef world) {}
            virtual void OnWorldShutdown(WorldRef world) {}

            virtual void OnPreWorldUpdate(WorldRef world, const SystemUpdateArgs& args) {}
            virtual void OnWorldUpdate(WorldRef world,  const SystemUpdateArgs& args) {}
            virtual void OnPostWorldUpdate(WorldRef world, const SystemUpdateArgs& args) {}

            virtual bool OnPreHandleWorldAction(WorldRef world, const SystemActionArgs& args) { return false; }
            virtual bool OnHandleWorldAction(WorldRef world, const SystemActionArgs& args) { return false; }
            virtual bool OnPostHandleWorldAction(WorldRef world, const SystemActionArgs& args) { return false; }

            virtual void OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer) {}

        protected:

            friend class FeatureECS;

            Session* Session = nullptr;
        };
    }
}

#define PHX_ECS_DECLARE_SYSTEM_BEGIN(type) PHX_DECLARE_DERIVED_TYPE_BEGIN(type, ECS::ISystem)
#define PHX_ECS_DECLARE_SYSTEM_END() PHX_DECLARE_DERIVED_TYPE_END()

#define PHX_ECS_DECLARE_SYSTEM(type) \
    PHX_ECS_DECLARE_SYSTEM_BEGIN(type) \
    PHX_ECS_DECLARE_SYSTEM_END()