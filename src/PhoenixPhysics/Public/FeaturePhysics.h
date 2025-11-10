#pragma once

#include "FeatureECS.h"
#include "PhysicsSystem.h"
#include "System.h"
#include "Containers/FixedArray.h"
#include "Containers/FixedSet.h"
#include "FixedPoint/FixedPoint.h"
#include "FixedPoint/FixedVector.h"
#include "FixedPoint/FixedLine.h"

namespace Phoenix
{
    namespace Physics
    {
        struct BodyComponent;

        struct PHOENIXSIM_API CollisionLine
        {
            Line2 Line;
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

        struct PHOENIXSIM_API FeaturePhysicsDynamicBlock : BufferBlockBase
        {
            PHX_DECLARE_BLOCK_SCRATCH(FeaturePhysicsDynamicBlock)

            bool bAllowSleep = true;
        };

        struct PHOENIXSIM_API FeaturePhysicsScratchBlock : BufferBlockBase
        {
            PHX_DECLARE_BLOCK_SCRATCH(FeaturePhysicsScratchBlock)

            uint64 NumIterations = 0;
            uint64 NumCollisions = 0;
            uint64 MaxQueryBodyCount = 0;

            TFixedArray<EntityBody, PHX_ECS_MAX_ENTITIES> SortedEntities;
            TFixedArray<Contact, PHX_ECS_MAX_ENTITIES> Contacts;
            TFixedSet<uint64, PHX_ECS_MAX_ENTITIES> ContactSet;
            TFixedArray<CollisionLine, 1000> CollisionLines;
        };

        class PHOENIXSIM_API FeaturePhysics : public IFeature
        {
            PHX_FEATURE_BEGIN(FeaturePhysics)
                FEATURE_WORLD_BLOCK(FeaturePhysicsDynamicBlock)
                FEATURE_WORLD_BLOCK(FeaturePhysicsScratchBlock)
                FEATURE_CHANNEL(FeatureChannels::HandleWorldAction)
                PHX_REGISTER_PROPERTY(bool, DebugDrawContacts)
                PHX_REGISTER_PROPERTY(bool, AllowSleep)
            PHX_FEATURE_END()

        public:

            FeaturePhysics();

            void Initialize() override;

            bool OnHandleWorldAction(WorldRef world, const FeatureActionArgs& action) override;

            static void QueryEntitiesInRange(WorldConstRef& world, const Vec2& pos, Distance range, TArray<EntityBody>& outEntities);

            static void AddExplosionForceToEntitiesInRange(
                WorldRef& world,
                const Vec2& pos,
                Distance range,
                Value force);

            static void AddForce(WorldRef& world, ECS::EntityId entityId, const Vec2& force);

            bool GetDebugDrawContacts() const;
            void SetDebugDrawContacts(const bool& value);

            bool GetAllowSleep() const;
            void SetAllowSleep(const bool& value);

            TSharedPtr<PhysicsSystem> PhysicsSystem;
        };
    }
}
