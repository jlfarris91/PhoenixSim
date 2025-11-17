
#pragma once

#include "DLLExport.h"
#include "System.h"

namespace Phoenix::Steering
{
    class PHOENIX_STEERING_API SteeringSystem : public ECS::ISystem
    {
    public:
        PHX_ECS_DECLARE_SYSTEM(SteeringSystem)

        void OnWorldUpdate(WorldRef world, const ECS::SystemUpdateArgs& args) override;

        void OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer) override;
    };
}
