
#include "FeaturePhysics.h"

#include <algorithm>

#include "MortonCode.h"
#include "Session.h"

using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;

const FName FeaturePhysics::StaticName = "Physics"_n;
const FName FeaturePhysicsScratchBlock::StaticName = "Physics_Scratch"_n;
//
// void AccessComponents2(WorldRef world, TFixedArray<TTuple<Entity*, BodyComponent*>, ECS_MAX_ENTITIES>& outEntityComponents)
// {
//     FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
//
//     outEntityComponents.Reset();
//
//     for (Entity& entity : block.Entities)
//     {
//         if (entity.Id == EntityId::Invalid)
//             continue;
//
//         auto components = std::make_tuple(&entity, FeatureECS::GetComponentDataPtr<BodyComponent>(world, entity.Id));
//         if (contains_nullptr(components))
//             continue;
//
//         outEntityComponents.PushBack(components);
//     }
// }

void PhysicsSystem::OnUpdate(WorldRef world)
{
    float dt = 1 / 60.0f;

    FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    auto& sortedEntities = scratchBlock.SortedEntities;

    // Calculate z-codes and sort entities
    {
        scratchBlock.SortedEntities.Reset();

        scratchBlock.EntityBodies.Refresh(world);

        for (auto && [entity, body] : scratchBlock.EntityBodies)
        {
            uint32 x = static_cast<uint32>(body->Transform.Position.X) >> MortonCodeGridBits;
            uint32 y = static_cast<uint32>(body->Transform.Position.Y) >> MortonCodeGridBits;
            uint64 zcode = MortonCode(x, y);
            sortedEntities.EmplaceBack(entity->Id, body, zcode);
        }

        // Sort entities by their zcodes
        std::ranges::sort(scratchBlock.SortedEntities,
            [](const EntityBody& a, const EntityBody& b)
            {
                return a.ZCode < b.ZCode;
            });
    }

    scratchBlock.NumIterations = 0;
    scratchBlock.NumCollisions = 0;
    scratchBlock.MaxQueryBodyCount = 0;
    scratchBlock.Contacts.Reset();
    scratchBlock.ContactSet.Reset();

    std::vector<TTuple<uint64, uint64>> ranges;

    // Determine contacts
    for (const EntityBody& entityBody : sortedEntities)
    {
        BodyComponent* bodyA = entityBody.BodyComponent;

        if (bodyA->Movement == EBodyMovement::Attached)
            continue;

        // Query for overlapping morton ranges
        {
            Distance radius = bodyA->Radius;
            uint32 lox = static_cast<uint32>(bodyA->Transform.Position.X - radius);
            uint32 hix = static_cast<uint32>(bodyA->Transform.Position.X + radius);
            uint32 loy = static_cast<uint32>(bodyA->Transform.Position.Y - radius);
            uint32 hiy = static_cast<uint32>(bodyA->Transform.Position.Y + radius);

            MortonCodeAABB aabb;
            aabb.MinX = lox >> MortonCodeGridBits;
            aabb.MinY = loy >> MortonCodeGridBits;
            aabb.MaxX = hix >> MortonCodeGridBits;
            aabb.MaxY = hiy >> MortonCodeGridBits;
        
            ranges.clear();
            MortonCodeQuery(aabb, ranges);
        }

        TArray<const EntityBody*> overlappingBodies;
        ForEachInMortonCodeRanges<EntityBody, &EntityBody::ZCode>(
            sortedEntities,
            ranges,
            [&](const EntityBody& eb)
            {
                overlappingBodies.push_back(&eb);
            });

        if (!overlappingBodies.empty())
        {
            scratchBlock.MaxQueryBodyCount = max(scratchBlock.MaxQueryBodyCount, overlappingBodies.size() - 1);
        }

        for (const EntityBody* entityBodyB : overlappingBodies)
        {
            BodyComponent* bodyB = entityBodyB->BodyComponent;

            if (scratchBlock.Contacts.IsFull())
            {
                continue;
            }

            if (bodyA == bodyB)
            {
                continue;
            }

            if ((bodyA->CollisionMask & bodyB->CollisionMask) == 0)
            {
                continue;
            }

            uint64 key = MortonCode(entityBody.EntityId, entityBodyB->EntityId);
            if (scratchBlock.ContactSet.Contains(key))
            {
                continue;
            }

            scratchBlock.ContactSet.Insert(key);
            
            ++scratchBlock.NumIterations;

            Vec2 v;
            if (Vec2::Equals(bodyA->Transform.Position, bodyB->Transform.Position))
            {
                v = Vec2::RandUnitVector();
                float correction = 1.0f / (bodyA->InvMass + bodyB->InvMass);
                bodyA->Transform.Position -= v * correction;
                bodyB->Transform.Position += v * correction;
            }

            v = bodyB->Transform.Position - bodyA->Transform.Position;
            
            float vLen = v.Length();
            float rr = bodyA->Radius + bodyB->Radius;
            if (vLen > rr)
            {
                continue;
            }
            
            const float baum = 0.3f;
            const float slop = 0.1f;
            float d = rr - vLen;
            float bias = baum * max(0, d - slop) / dt;
            float effMass = 1.0f / (bodyA->InvMass + bodyB->InvMass);

            Contact& contact = scratchBlock.Contacts.AddDefaulted_GetRef();
            contact.BodyA = bodyA;
            contact.BodyB = bodyB;
            contact.Normal = v.Normalized();
            contact.Bias = bias;
            contact.EffMass = effMass;
            contact.Impulse = 0;

            ++scratchBlock.NumCollisions;
        }
    }

    // Multi-pass solver
    for (uint32 iter = 0; iter < 2; ++iter)
    {
        for (Contact& contact : scratchBlock.Contacts)
        {
            // Relative velocity at contact
            float relVel = Vec2::Dot(contact.Normal, contact.BodyB->Velocity - contact.BodyA->Velocity);

            // Compute corrective impulse
            float lambda = -(relVel + contact.Bias) * contact.EffMass;

            // Accumulate and project (no negative normal impulses)
            float oldImpulse = contact.Impulse;
            contact.Impulse = max(oldImpulse + lambda, 0.0f);
            float change = contact.Impulse - oldImpulse;

            // Apply impulse
            Vec2 p = contact.Normal * change;
            contact.BodyA->Velocity -= p * contact.BodyA->InvMass;
            contact.BodyB->Velocity += p * contact.BodyB->InvMass;
        }   
    }

    // Multi-pass resolve overlaps
    for (uint32 i = 0; i < 2; ++i)
    {
        for (Contact& contact : scratchBlock.Contacts)
        {
            Vec2 v = contact.BodyB->Transform.Position - contact.BodyA->Transform.Position;
            float d = v.Length();
            float rr = contact.BodyA->Radius + contact.BodyB->Radius;
            float pen = rr - d;
            if (pen > 0.01)
            {
                float correction = 0.3f * pen;
                contact.BodyA->Transform.Position -= contact.Normal * correction * contact.BodyA->InvMass / (contact.BodyA->InvMass + contact.BodyB->InvMass);
                contact.BodyB->Transform.Position += contact.Normal * correction * contact.BodyB->InvMass / (contact.BodyA->InvMass + contact.BodyB->InvMass);
            }
        }
    }

    for (const EntityBody& entityBody : sortedEntities)
    {
        BodyComponent* body = entityBody.BodyComponent;

        if (body->Movement == EBodyMovement::Attached)
        {
            body->Transform.Rotation += 10.0f;
        }
        else
        {
            body->Transform.Position += body->Velocity * dt;
        }
    }
}

FeaturePhysics::FeaturePhysics()
{
    WorldBufferBlockArgs blockArgs;
    blockArgs.Name = FeaturePhysicsScratchBlock::StaticName;
    blockArgs.Size = sizeof(FeaturePhysicsScratchBlock);
    blockArgs.BlockType = EWorldBufferBlockType::Scratch;

    FeatureDefinition.Name = StaticName;
    FeatureDefinition.Blocks.push_back(blockArgs);

    FeatureDefinition.Channels.emplace_back(WorldChannels::HandleAction, FeatureInsertPosition::Default);
}

FName FeaturePhysics::GetName() const
{
    return StaticName;
}

FeatureDefinition FeaturePhysics::GetFeatureDefinition()
{
    return FeatureDefinition;
}

void FeaturePhysics::Initialize()
{
    std::shared_ptr<PhysicsSystem> physicsSystem = std::make_shared<PhysicsSystem>();

    TSharedPtr<FeatureECS> featureECS = Session->GetFeatureSet()->GetFeature<FeatureECS>();
    featureECS->RegisterSystem(physicsSystem);
}

void FeaturePhysics::OnHandleAction(WorldRef world, const FeatureActionArgs& action)
{
    IFeature::OnHandleAction(world, action);

    if (action.Action.Verb == "release_entities_in_range"_n)
    {
        Vec2 pos = { action.Action.Data[0].Distance, action.Action.Data[1].Distance };
        Distance range = action.Action.Data[2].Distance;

        TArray<EntityId> outEntities;
        QueryEntitiesInRange(world, pos, range, outEntities);

        for (EntityId entityId : outEntities)
        {
            FeatureECS::ReleaseEntity(world, entityId);
        }
    }
}

void FeaturePhysics::QueryEntitiesInRange(
    WorldConstRef world,
    const Vec2& pos,
    Distance range,
    TArray<EntityId>& outEntities)
{
    const FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    TArray<TTuple<uint64, uint64>> ranges;
    
    // Query for overlapping morton ranges
    {
        uint32 lox = static_cast<uint32>(pos.X - range);
        uint32 hix = static_cast<uint32>(pos.X + range);
        uint32 loy = static_cast<uint32>(pos.Y - range);
        uint32 hiy = static_cast<uint32>(pos.Y + range);

        MortonCodeAABB aabb;
        aabb.MinX = lox >> MortonCodeGridBits;
        aabb.MinY = loy >> MortonCodeGridBits;
        aabb.MaxX = hix >> MortonCodeGridBits;
        aabb.MaxY = hiy >> MortonCodeGridBits;

        MortonCodeQuery(aabb, ranges);
    }

    TArray<EntityBody*> overlappingEntities;
    ForEachInMortonCodeRanges<EntityBody, &EntityBody::ZCode>(
        scratchBlock.SortedEntities,
        ranges,
        [&](const EntityBody& entityZCode)
        {
            outEntities.push_back(entityZCode.EntityId);
        });
}
