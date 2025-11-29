
#pragma once

#include "Component.h"
#include "DLLExport.h"
#include "Platform.h"
#include "FixedPoint/FixedTypes.h"
#include "FixedPoint/FixedVector.h"

namespace Phoenix
{
    namespace Physics
    {
        enum class PHOENIX_PHYSICS_API EBodyMovement : uint8
        {
            Idle,
            Moving,
            Attached
        };

        enum class PHOENIX_PHYSICS_API EBodyFlags : uint8
        {
            None = 0,
            Awake = 1,
            StaticX = 2,
            StaticY = 4,
            Static = StaticX | StaticY
        };

        struct PHOENIX_PHYSICS_API BodyComponent : ECS::IComponent
        {
            PHX_ECS_DECLARE_COMPONENT(BodyComponent)

            EBodyFlags Flags = EBodyFlags::None; 

            // Collision flags.
            uint16 CollisionMask = 0;

            // The state of the body.
            EBodyMovement Movement = EBodyMovement::Idle;

            // The radius used for body separation and pathfinding.
            Distance Radius = 0;

            // Accumulated over a frame and reset to 0 after integration.
            Vec2 Force;

            // The amount of distance applied to the relative transform each step.
            Vec2 LinearVelocity = Vec2::Zero;

            Value LinearDamping = 0;

            // The mass of the body. Used when resolving body separation.
            InvValue InvMass;

            uint8 SleepTimer = 0;
        };
        
    }
}
