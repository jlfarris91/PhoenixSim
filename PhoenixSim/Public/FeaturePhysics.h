#pragma once

#include "FeatureECS.h"
#include "FixedArray.h"
#include "PhoenixSim.h"
#include "FixedPoint.h"
#include "FixedSet.h"

namespace Phoenix
{
    namespace Physics
    {
        PHOENIXSIM_API enum class EBodyMovement : uint8
        {
            Idle,
            Moving,
            Attached
        };

        struct PHOENIXSIM_API BodyComponent
        {
            static constexpr FName StaticTypeName = "Body"_n;

            // Collision flags.
            uint32 CollisionMask = 0;

            // The state of the body.
            EBodyMovement Movement = EBodyMovement::Idle;

            // The id of the entity this body is attached to.
            // Note that this is not the entity that owns the body component.
            ECS::EntityId AttachParent = ECS::EntityId::Invalid;

            // The relative transform of the body.
            // Relative to the origin if not attached to another entity body.
            Transform2D Transform;

            // The radius used for body separation and pathfinding.
            Distance Radius = 0;

            // The amount of distance applied to the relative transform each step.
            Vec2 Velocity = Vec2::Zero;
            float Speed = 0;

            // The mass of the body. Used when resolving body separation.
            Mass InvMass = 0;

            // Start and end points used for interpolation.
            Vec2 Steps[2];

            // Interpolation value between Steps.
            Value StepT = Zero<Value>();
        };

        class PHOENIXSIM_API PhysicsSystem : public ECS::ISystem
        {
        public:
            void OnUpdate(WorldRef world) override;
        };

        struct EntityBody
        {
            ECS::EntityId EntityId;
            BodyComponent* BodyComponent;
            uint64 ZCode;
        };

        struct Contact
        {
            BodyComponent* BodyA;
            BodyComponent* BodyB;
            Vec2 Normal;
            Value EffMass;
            Value Bias;
            Value Impulse;
        };

        struct PHOENIXSIM_API FeaturePhysicsScratchBlock
        {
            static const FName StaticName;
            uint64 NumIterations = 0;
            uint64 NumCollisions = 0;
            uint64 MaxQueryBodyCount = 0;

            ECS::EntityComponentsContainer<BodyComponent> EntityBodies;
            TFixedArray<EntityBody, ECS_MAX_ENTITIES> SortedEntities;
            TFixedArray<Contact, ECS_MAX_ENTITIES> Contacts;
            TFastSet<uint64, ECS_MAX_ENTITIES> ContactSet;
        };

        class PHOENIXSIM_API FeaturePhysics : public IFeature
        {
        public:

            static const FName StaticName;

            FeaturePhysics();

            FName GetName() const override;

            FeatureDefinition GetFeatureDefinition() override;

            void Initialize() override;

            void OnHandleAction(WorldRef world, const FeatureActionArgs& action) override;

            static void QueryEntitiesInRange(WorldConstRef& world, const Vec2& pos, Distance range, TArray<ECS::EntityId>& outEntities);

        private:

            FeatureDefinition FeatureDefinition;
        };
    }    
}
