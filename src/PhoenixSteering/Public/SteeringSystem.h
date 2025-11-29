
#pragma once

#include "DLLExport.h"
#include "System.h"

namespace Phoenix::Steering
{
    class PHOENIX_STEERING_API SteeringSystem : public ECS::ISystem
    {
    public:
        PHX_ECS_DECLARE_SYSTEM_BEGIN(SteeringSystem)
            PHX_REGISTER_FIELD(bool, MoveTowardsGoal)
            PHX_REGISTER_FIELD(double, DensityScalar)
            PHX_REGISTER_FIELD(double, DensityRadiusScalar)
            PHX_REGISTER_FIELD(double, AvoidanceScalar)
            PHX_REGISTER_FIELD(double, AvoidanceRadiusScalar)
        PHX_ECS_DECLARE_SYSTEM_END()

        void OnPreWorldUpdate(WorldRef world, const ECS::SystemUpdateArgs& args) override;
        void OnWorldUpdate(WorldRef world, const ECS::SystemUpdateArgs& args) override;

        void OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer) override;

        bool MoveTowardsGoal = true;
        double DensityScalar = 0.3;
        double DensityRadiusScalar = 2.2;
        double AvoidanceScalar = 1.3;
        double AvoidanceRadiusScalar = 2.2;
    };
}
