
#include "FeaturePhysics.h"

#include <algorithm>

#include "FeatureTrace.h"
#include "Flags.h"
#include "MortonCode.h"
#include "Session.h"

using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;

const FName FeaturePhysics::StaticName = "Physics"_n;
const FName FeaturePhysicsScratchBlock::StaticName = "Physics_Scratch"_n;

constexpr uint8 SLEEP_TIMER = 1;

FName PhysicsSystem::GetName()
{
    return StaticName;
}

void PhysicsSystem::OnPreUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    scratchBlock.NumIterations = 0;
    scratchBlock.NumCollisions = 0;
    scratchBlock.MaxQueryBodyCount = 0;
    scratchBlock.Contacts.Reset();
    scratchBlock.ContactSet.Reset();
    
    // Gather all transform + body component pairs
    {
        ScopedTrace trace(world, "GatherBodyComponents"_n);
        scratchBlock.EntityBodies.Refresh(world);
    }

    // Calculate z-codes and sort entities
    {
        ScopedTrace trace(world, "CalculateAndSortZCodes"_n);
        scratchBlock.SortedEntities.Reset();
        for (auto && [entity, transformComp, bodyComp] : scratchBlock.EntityBodies)
        {
            uint32 x = static_cast<uint32>(transformComp->Transform.Position.X) >> MortonCodeGridBits;
            uint32 y = static_cast<uint32>(transformComp->Transform.Position.Y) >> MortonCodeGridBits;
            uint64 zcode = MortonCode(x, y);
            scratchBlock.SortedEntities.EmplaceBack(entity->Id, transformComp, bodyComp, zcode);
        }

        // Sort entities by their zcodes
        std::ranges::sort(scratchBlock.SortedEntities,
            [](const EntityBody& a, const EntityBody& b)
            {
                return a.ZCode < b.ZCode;
            });
    }

}

void PhysicsSystem::OnUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    Value dt = 1.0f / args.StepHz;

    FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    std::vector<TTuple<uint64, uint64>> ranges;

    // Determine contacts
    {
        ScopedTrace trace(world, "CalculateContacts"_n);
        for (const EntityBody& entityBodyA : scratchBlock.SortedEntities)
        {
            TransformComponent* transformCompA = entityBodyA.TransformComponent;
            BodyComponent* bodyCompA = entityBodyA.BodyComponent;

            if (!HasFlag(bodyCompA->Flags, EBodyFlags::Awake))
            {
                continue;
            }

            // Query for overlapping morton ranges
            TArray<const EntityBody*> overlappingBodies;
            {
                ScopedTrace trace2(world, "OverlapQuery"_n);

                Distance radius = bodyCompA->Radius;
                uint32 lox = static_cast<uint32>(transformCompA->Transform.Position.X - radius);
                uint32 hix = static_cast<uint32>(transformCompA->Transform.Position.X + radius);
                uint32 loy = static_cast<uint32>(transformCompA->Transform.Position.Y - radius);
                uint32 hiy = static_cast<uint32>(transformCompA->Transform.Position.Y + radius);

                MortonCodeAABB aabb;
                aabb.MinX = lox >> MortonCodeGridBits;
                aabb.MinY = loy >> MortonCodeGridBits;
                aabb.MaxX = hix >> MortonCodeGridBits;
                aabb.MaxY = hiy >> MortonCodeGridBits;
        
                ranges.clear();
                MortonCodeQuery(aabb, ranges);

                ForEachInMortonCodeRanges<EntityBody, &EntityBody::ZCode>(
                    scratchBlock.SortedEntities,
                    ranges,
                    [&](const EntityBody& eb)
                    {
                        overlappingBodies.push_back(&eb);
                    });
            }

            if (!overlappingBodies.empty())
            {
                scratchBlock.MaxQueryBodyCount = max(scratchBlock.MaxQueryBodyCount, overlappingBodies.size() - 1);
            }

            for (const EntityBody* entityBodyB : overlappingBodies)
            {
                TransformComponent* transformCompB = entityBodyB->TransformComponent;
                BodyComponent* bodyCompB = entityBodyB->BodyComponent;

                if (scratchBlock.Contacts.IsFull())
                {
                    continue;
                }

                if (bodyCompA == bodyCompB)
                {
                    continue;
                }

                if ((bodyCompA->CollisionMask & bodyCompB->CollisionMask) == 0)
                {
                    continue;
                }

                EntityId loId = min(entityBodyA.EntityId, entityBodyB->EntityId);
                EntityId hiId = max(entityBodyA.EntityId, entityBodyB->EntityId);

                uint64 key = MortonCode(loId, hiId);
                if (scratchBlock.ContactSet.Contains(key))
                {
                    continue;
                }

                {
                    ScopedTrace trace2(world, "ContactSetQuery"_n);
                    scratchBlock.ContactSet.Insert(key);
                }
            
                ++scratchBlock.NumIterations;

                Vec2 v;
                if (Vec2::Equals(transformCompA->Transform.Position, transformCompB->Transform.Position))
                {
                    v = Vec2::RandUnitVector();
                    float correction = 1.0f / (bodyCompA->InvMass + bodyCompB->InvMass);
                    transformCompA->Transform.Position -= v * correction;
                    transformCompB->Transform.Position += v * correction;
                }

                v = transformCompB->Transform.Position - transformCompA->Transform.Position;
            
                float vLen = v.Length();
                float rr = bodyCompA->Radius + bodyCompB->Radius;
                if (vLen > rr)
                {
                    continue;
                }
            
                const float baum = 0.3f;
                const float slop = 0.5f;
                float d = rr - vLen;
                float bias = baum * max(0, d - slop) / dt;
                float effMass = 1.0f / (bodyCompA->InvMass + bodyCompB->InvMass);

                Contact& contact = scratchBlock.Contacts.AddDefaulted_GetRef();
                contact.TransformA = transformCompA;
                contact.BodyA = bodyCompA;
                contact.TransformB = transformCompB;
                contact.BodyB = bodyCompB;
                contact.Normal = v.Normalized();
                contact.Bias = bias;
                contact.EffMass = effMass;
                contact.Impulse = 0;

                SetFlagRef(bodyCompA->Flags, EBodyFlags::Awake, true);
                SetFlagRef(bodyCompB->Flags, EBodyFlags::Awake, true);

                ++scratchBlock.NumCollisions;
            }
        }
    }

    // Multi-pass solver
    {
        ScopedTrace trace(world, "PGS"_n);
        for (uint32 iter = 0; iter < 2; ++iter)
        {
            for (Contact& contact : scratchBlock.Contacts)
            {
                // Relative velocity at contact
                float relVel = Vec2::Dot(contact.Normal, contact.BodyB->LinearVelocity - contact.BodyA->LinearVelocity);

                // Compute corrective impulse
                float lambda = -(relVel + contact.Bias) * contact.EffMass;

                // Accumulate and project (no negative normal impulses)
                float oldImpulse = contact.Impulse;
                contact.Impulse = max(oldImpulse + lambda, 0.0f);
                float change = contact.Impulse - oldImpulse;

                // Apply impulse
                Vec2 p = contact.Normal * change;
                contact.BodyA->LinearVelocity -= p * contact.BodyA->InvMass;
                contact.BodyB->LinearVelocity += p * contact.BodyB->InvMass;
            }   
        }
    }

    // Multi-pass resolve overlaps
    {
        ScopedTrace trace(world, "ResolveOverlaps"_n);
        for (uint32 i = 0; i < 2; ++i)
        {
            for (Contact& contact : scratchBlock.Contacts)
            {
                Vec2 v = contact.TransformB->Transform.Position - contact.TransformA->Transform.Position;
                float d = v.Length();
                float rr = contact.BodyA->Radius + contact.BodyB->Radius;
                float pen = rr - d;
                if (pen > 0.01)
                {
                    float correction = 0.3f * pen;
                    contact.TransformA->Transform.Position -= contact.Normal * correction * contact.BodyA->InvMass / (contact.BodyA->InvMass + contact.BodyB->InvMass);
                    contact.TransformB->Transform.Position += contact.Normal * correction * contact.BodyB->InvMass / (contact.BodyA->InvMass + contact.BodyB->InvMass);

                    SetFlagRef(contact.BodyA->Flags, EBodyFlags::Awake, true);
                    SetFlagRef(contact.BodyB->Flags, EBodyFlags::Awake, true);

                    contact.BodyA->SleepTimer = SLEEP_TIMER;
                    contact.BodyB->SleepTimer = SLEEP_TIMER;
                }
            }
        }
    }

    // Integrate velocities
    {
        ScopedTrace trace(world, "Integrate"_n);
        for (const EntityBody& entityBody : scratchBlock.SortedEntities)
        {
            BodyComponent* bodyComp = entityBody.BodyComponent;
            TransformComponent* transformComp = entityBody.TransformComponent;

            if (bodyComp->Movement == EBodyMovement::Attached)
            {
                transformComp->Transform.Rotation += 10.0f;
            }
            else
            {
                bool isMoving = bodyComp->LinearVelocity.LengthSq() > (0.01f * 0.01f);
                if (isMoving)
                {
                    bodyComp->SleepTimer = SLEEP_TIMER;
                    SetFlagRef(bodyComp->Flags, EBodyFlags::Awake, true);
                }
                else if (bodyComp->SleepTimer > 0)
                {
                    --bodyComp->SleepTimer;
                    SetFlagRef(bodyComp->Flags, EBodyFlags::Awake, true);
                }
                else
                {
                    SetFlagRef(bodyComp->Flags, EBodyFlags::Awake, false);
                }
                
                transformComp->Transform.Position += bodyComp->LinearVelocity * dt;
                bodyComp->LinearVelocity *= (1.0f - bodyComp->LinearDamping * dt);
            }
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
        Distance rangeSq = range * range;

        TArray<EntityBody> outEntities;
        QueryEntitiesInRange(world, pos, range, outEntities);

        for (const EntityBody& entityBody : outEntities)
        {
            const Vec2& entityPos = entityBody.TransformComponent->Transform.Position;
            if (Vec2::DistanceSq(pos, entityPos) < rangeSq)
            {
                FeatureECS::ReleaseEntity(world, entityBody.EntityId);
            }
        }
    }

    if (action.Action.Verb == "push_entities_in_range"_n)
    {
        Vec2 pos = { action.Action.Data[0].Distance, action.Action.Data[1].Distance };
        Distance range = action.Action.Data[2].Distance;
        Value force = action.Action.Data[3].Distance;
        AddExplosionForceToEntitiesInRange(world, pos, range, force);
    }
}

void FeaturePhysics::QueryEntitiesInRange(
    WorldConstRef world,
    const Vec2& pos,
    Distance range,
    TArray<EntityBody>& outEntities)
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
        [&](const EntityBody& entityBody)
        {
            outEntities.push_back(entityBody);
        });
}

void FeaturePhysics::AddExplosionForceToEntitiesInRange(WorldRef world, const Vec2& pos, Distance range, Value force)
{
    Distance rangeSq = range * range;

    TArray<EntityBody> outEntities;
    QueryEntitiesInRange(world, pos, range, outEntities);

    for (const EntityBody& entityBody : outEntities)
    {
        const Vec2& entityPos = entityBody.TransformComponent->Transform.Position;
        Vec2 dir = entityPos - pos;
        Distance distSq = dir.LengthSq();
        if (distSq < rangeSq)
        {
            Value t = 1.0f - distSq / rangeSq;
            Value f = force / entityBody.BodyComponent->InvMass;
            entityBody.BodyComponent->LinearVelocity += dir.Normalized() * f * t;
        }
    }
}
