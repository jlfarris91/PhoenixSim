
#pragma once

#include "DLLExport.h"
#include "Component.h"
#include "EntityId.h"
#include "FixedPoint/FixedVector.h"

namespace Phoenix::Steering
{
    enum class ESeekFlags
    {
        None = 0,
        Arrive = 1,
        Flee = 2,
    };
    
    struct PHOENIX_STEERING_API SeekComponent : ECS::IComponent
    {
        PHX_ECS_DECLARE_COMPONENT(SeekComponent)

        ESeekFlags Flags = ESeekFlags::None;
        ECS::EntityId TargetEntity;
        Vec2 TargetPos;
        Speed MaxSpeed;
        Distance SlowingDistance;
    };
    
    struct PHOENIX_STEERING_API WanderComponent : ECS::IComponent
    {
        PHX_ECS_DECLARE_COMPONENT(WanderComponent)

        Angle WanderAngle;
        Distance WanderRadius;
        Speed MaxSpeed;
    };
}
