
#pragma once

#include "DLLExport.h"
#include "System.h"

namespace Phoenix::Physics
{
    class PHOENIX_PHYSICS_API PhysicsSystem : public ECS::ISystem
    {
    public:
        PHX_ECS_DECLARE_SYSTEM_BEGIN(PhysicsSystem)
            PHX_REGISTER_FIELD(bool, DebugDrawContacts)
            PHX_REGISTER_FIELD(bool, AllowSleep)
            PHX_REGISTER_FIELD(uint8, NumIterations)
            PHX_REGISTER_FIELD(uint8, NumSolverSteps)
            PHX_REGISTER_FIELD(uint8, NumSeparationSteps)
            PHX_REGISTER_FIELD(double, PenetrationThreshold)
            PHX_REGISTER_FIELD(double, PenetrationCorrection)
        PHX_ECS_DECLARE_SYSTEM_END()

        void OnPreWorldUpdate(WorldRef world, const ECS::SystemUpdateArgs& args) override;
        void OnWorldUpdate(WorldRef world, const ECS::SystemUpdateArgs& args) override;
        void OnPostWorldUpdate(WorldRef world, const ECS::SystemUpdateArgs& args) override;

        void OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer) override;

        bool DebugDrawContacts = false;
        bool AllowSleep = true;
        uint8 NumIterations = 2;
        uint8 NumSolverSteps = 6;
        uint8 NumSeparationSteps = 40;
        double PenetrationThreshold = 0.05;
        double PenetrationCorrection = 0.1;
    };
}
