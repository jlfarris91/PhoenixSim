#pragma once

#include "FeatureECS.h"
#include "PhysicsSystem.h"
#include "System.h"
#include "Containers/FixedArray.h"
#include "Containers/FixedSet.h"
#include "FixedPoint/FixedPoint.h"
#include "FixedPoint/FixedVector.h"
#include "FixedPoint/FixedLine.h"

#ifndef PHX_PHS_MAX_CONTACTS_PER_ENTITY
#define PHX_PHS_MAX_CONTACTS_PER_ENTITY 8
#endif

#ifndef PHX_PHS_MAX_CONTACTS
#define PHX_PHS_MAX_CONTACTS (PHX_ECS_MAX_ENTITIES * PHX_PHS_MAX_CONTACTS_PER_ENTITY)
#endif

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

        struct ContactPair
        {
            uint64 Key;
            ECS::TransformComponent* TransformA;
            BodyComponent* BodyA;
            ECS::TransformComponent* TransformB;
            BodyComponent* BodyB;
        };

        struct Contact
        {
            uint32 ContactPair;
            Vec2 Normal;
            Value EffMass;
            Value Bias;
            Value Impulse;
        };

        struct ContactPairHasher
        {
            uint64 operator()(uint64 v) const
            {
                // Murmur hash
                uint64_t h = v;
                h ^= h >> 33;
                h *= 0xff51afd7ed558ccduLL;
                h ^= h >> 33;
                h *= 0xc4ceb9fe1a85ec53uLL;
                h ^= h >> 33;
                return h;
            }
        };

        struct PHOENIXSIM_API FeaturePhysicsDynamicBlock : BufferBlockBase
        {
            PHX_DECLARE_BLOCK_SCRATCH(FeaturePhysicsDynamicBlock)

            bool bAllowSleep = true;
        };

        struct PHOENIXSIM_API FeaturePhysicsScratchBlock : BufferBlockBase
        {
            PHX_DECLARE_BLOCK_SCRATCH(FeaturePhysicsScratchBlock)

            TFixedArray<EntityBody, PHX_ECS_MAX_ENTITIES> SortedEntities;
            TAtomic<uint32> SortedEntityCount = 0;

            TFixedArray<ContactPair, PHX_PHS_MAX_CONTACTS> ContactPairs;
            TAtomic<uint32> ContactPairsCount = 0;

            TFixedArray<Contact, PHX_PHS_MAX_CONTACTS> Contacts;
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
