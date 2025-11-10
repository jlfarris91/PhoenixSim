
#pragma once

#include "DLLExport.h"
#include "System.h"

namespace Phoenix
{
    namespace Physics
    {
        class PHOENIX_PHYSICS_API PhysicsSystem : public ECS::ISystem
        {
        public:
            PHX_ECS_DECLARE_SYSTEM(PhysicsSystem)

            void OnPreWorldUpdate(WorldRef world, const ECS::SystemUpdateArgs& args) override;
            void OnWorldUpdate(WorldRef world, const ECS::SystemUpdateArgs& args) override;

            void OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer) override;

            bool bDebugDrawContacts = false;
        };
    }
}
