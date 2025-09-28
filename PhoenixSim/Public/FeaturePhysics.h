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
        enum class PHOENIXSIM_API EBodyMovement : uint8
        {
            Idle,
            Moving,
            Attached
        };

        enum class PHOENIXSIM_API EBodyFlags : uint8
        {
            None = 0,
            Awake = 1,
            StaticX = 2,
            StaticY = 4,
            Static = StaticX | StaticY
        };

        struct PHOENIXSIM_API BodyComponent
        {
            static constexpr FName StaticName = "Body"_n;

            EBodyFlags Flags = EBodyFlags::None; 

            // Collision flags.
            uint16 CollisionMask = 0;

            // The state of the body.
            EBodyMovement Movement = EBodyMovement::Idle;

            // The radius used for body separation and pathfinding.
            Distance Radius = 0;

            // The amount of distance applied to the relative transform each step.
            Vec2 LinearVelocity = Vec2::Zero;

            Value LinearDamping = 0;

            // The mass of the body. Used when resolving body separation.
            TInvFixed2<Value> InvMass;

            uint8 SleepTimer = 0;
        };

        struct PHOENIXSIM_API CollisionLine
        {
            Line2 Line;
        };

        class PHOENIXSIM_API PhysicsSystem : public ECS::ISystem
        {
        public:
            static constexpr FName StaticName = "Physics"_n;
            FName GetName() override;
            void OnPreUpdate(WorldRef world, const ECS::SystemUpdateArgs& args) override;
            void OnUpdate(WorldRef world, const ECS::SystemUpdateArgs& args) override;
        };

        struct EntityBody
        {
            ECS::EntityId EntityId;
            ECS::TransformComponent* TransformComponent;
            BodyComponent* BodyComponent;
            uint64 ZCode;
        };

        struct Contact
        {
            ECS::TransformComponent* TransformA;
            BodyComponent* BodyA;
            ECS::TransformComponent* TransformB;
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

            ECS::EntityComponentsContainer<ECS::TransformComponent, BodyComponent> EntityBodies;
            TFixedArray<EntityBody, ECS_MAX_ENTITIES> SortedEntities;
            TFixedArray<Contact, ECS_MAX_ENTITIES> Contacts;
            TFastSet<uint64, ECS_MAX_ENTITIES> ContactSet;
            TFixedArray<CollisionLine, 1000> CollisionLines;

            ECS::EntityComponentsContainer<ECS::TransformComponent, BodyComponent, ECS::MovementComponent> MoveBodies;
            Vec2 MapCenter = Vec2::Zero;
            Angle Rotation = 0;
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

            static void QueryEntitiesInRange(WorldConstRef& world, const Vec2& pos, Distance range, TArray<EntityBody>& outEntities);

            static void AddExplosionForceToEntitiesInRange(
                WorldRef& world,
                const Vec2& pos,
                Distance range,
                Value force);

        private:

            FeatureDefinition FeatureDefinition;
        };
    }    
}
