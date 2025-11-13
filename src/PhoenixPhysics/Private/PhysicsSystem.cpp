
#include "PhysicsSystem.h"

#include <execution>

#include "BodyComponent.h"
#include "Color.h"
#include "Debug.h"
#include "FeaturePhysics.h"
#include "Flags.h"
#include "MortonCode.h"
#include "Profiling.h"

using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;

constexpr uint8 SLEEP_TIMER = 1;

struct SortEntitiesByZCodeJob : IBufferJob<TransformComponent&, BodyComponent&>
{
    void Execute(const EntityComponentSpan<TransformComponent&, BodyComponent&>& span) override
    {
        PHX_PROFILE_ZONE_SCOPED_N("SortBodiesByZCodeTask");

        FeaturePhysicsScratchBlock& scratchBlock = World->GetBlockRef<FeaturePhysicsScratchBlock>();

        for (auto && [entityId, index, transformComp, bodyComp] : span)
        {
            scratchBlock.SortedEntities[span.GetGlobalIndex(index)] = EntityBody{entityId, &transformComp, &bodyComp, transformComp.ZCode};
        }

        using iter = decltype(scratchBlock.SortedEntities)::Iter;
        iter startIter = iter(&scratchBlock.SortedEntities[0] + span.StartingIndex);
        iter endIter = iter(&scratchBlock.SortedEntities[0] + span.StartingIndex + span.InstanceCount);

        std::sort(
            startIter,
            endIter,
            [](const EntityBody& a, const EntityBody& b)
            {
                return a.ZCode < b.ZCode;
            });

        scratchBlock.SortedEntityCount += span.InstanceCount;
    }
};

struct CalculateContactPairsJob : IBufferJob<TransformComponent&, BodyComponent&>
{
    DeltaTime DeltaTime;

    void Execute(const EntityComponentSpan<TransformComponent&, BodyComponent&>& span) override
    {
        PHX_PROFILE_ZONE_SCOPED_N("CalculateContactPairsJob");

        FeaturePhysicsScratchBlock& scratchBlock = World->GetBlockRef<FeaturePhysicsScratchBlock>();
                
        TMortonCodeRangeArray ranges;

        const EntityBody* overlappingBodies[PHX_PHS_MAX_CONTACTS_PER_ENTITY * 4];
        uint32 overlappingBodiesCount = 0;

        for (auto && [entityIdA, index, transformCompA, bodyCompA] : span)
        {
            if (!HasAnyFlags(bodyCompA.Flags, EBodyFlags::Awake))
            {
                continue;
            }

            // Query for overlapping morton ranges
            {
                PHX_PROFILE_ZONE_SCOPED_N("OverlapQuery");

                Vec2 projectedPos = transformCompA.Transform.Position + bodyCompA.LinearVelocity * DeltaTime;

                MortonCodeAABB aabb = ToMortonCodeAABB(projectedPos, bodyCompA.Radius);
        
                ranges.clear();
                MortonCodeQuery(aabb, ranges);

                overlappingBodiesCount = 0;
                ForEachInMortonCodeRanges<EntityBody, &EntityBody::ZCode>(
                    scratchBlock.SortedEntities,
                    ranges,
                    [&](const EntityBody& eb)
                    {
                        if (eb.EntityId == entityIdA)
                        {
                            return false;
                        }

                        if ((bodyCompA.CollisionMask & eb.BodyComponent->CollisionMask) == 0)
                        {
                            return false;
                        }

                        overlappingBodies[overlappingBodiesCount++] = &eb;
                        return overlappingBodiesCount == _countof(overlappingBodies);
                    });
            }

            for (uint32 i = 0; i < overlappingBodiesCount; ++i)
            {
                const EntityBody* entityBodyB = overlappingBodies[i];
                TransformComponent& transformCompB = *entityBodyB->TransformComponent;
                BodyComponent& bodyCompB = *entityBodyB->BodyComponent;

                Vec2 v = transformCompB.Transform.Position - transformCompA.Transform.Position;            
                Distance vLen = v.Length();
                Distance rr = bodyCompA.Radius + bodyCompB.Radius;
                if (vLen > rr)
                {
                    continue;
                }

                entityid_t loId = Min(entityIdA, entityBodyB->EntityId);
                entityid_t hiId = Max(entityIdA, entityBodyB->EntityId);
                uint64 key = hiId; key = key << 32 | loId;

                uint32 contactIndex = scratchBlock.ContactPairsCount.fetch_add(1);
                if (contactIndex >= scratchBlock.ContactPairs.Capacity)
                    break;

                ContactPair& pair = scratchBlock.ContactPairs[contactIndex];
                pair.Key = key;
                pair.TransformA = &transformCompA;
                pair.BodyA = &bodyCompA;
                pair.TransformB = &transformCompB;
                pair.BodyB = &bodyCompB;
            }
        }
    }
};

struct IntegrateJob : IBufferJob<TransformComponent&, BodyComponent&>
{
    DeltaTime DeltaTime;

    void Execute(const EntityComponentSpan<TransformComponent&, BodyComponent&>& span) override
    {
        PHX_PROFILE_ZONE_SCOPED_N("IntegrateJob");

        FeaturePhysicsDynamicBlock& dynamicBlock = World->GetBlockRef<FeaturePhysicsDynamicBlock>();

        for (auto && [entityIdA, index, transformComp, bodyComp] : span)
        {
            if (bodyComp.Movement == EBodyMovement::Attached)
            {
                transformComp.Transform.Rotation += 10.0f;
            }
            else 
            {
                if (dynamicBlock.bAllowSleep)
                {
                    bool isMoving = bodyComp.LinearVelocity.Length() > Distance(1E-1);
                    if (isMoving)
                    {
                        bodyComp.SleepTimer = SLEEP_TIMER;
                        SetFlagRef(bodyComp.Flags, EBodyFlags::Awake, true);
                    }
                    else if (bodyComp.SleepTimer > 0)
                    {
                        --bodyComp.SleepTimer;
                        SetFlagRef(bodyComp.Flags, EBodyFlags::Awake, true);
                    }
                    else
                    {
                        SetFlagRef(bodyComp.Flags, EBodyFlags::Awake, false);
                    }
                }
                
                transformComp.Transform.Position += bodyComp.LinearVelocity * DeltaTime;

                bodyComp.LinearVelocity *= (1.0f - bodyComp.LinearDamping * DeltaTime);
            }
        }
    }    
};

void PhysicsSystem::OnPreWorldUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    // Sort entity bodies by z-code (calculated by FeatureECS)
    {
        PHX_PROFILE_ZONE_SCOPED_N("SortBodiesByZCode");

        scratchBlock.SortedEntities.Reset();
        scratchBlock.SortedEntityCount = 0;

        SortEntitiesByZCodeJob job;
        FeatureECS::ScheduleParallel(world, job);

        FeatureECS::ExecutePendingJobs(world);

        scratchBlock.SortedEntities.SetSize(scratchBlock.SortedEntityCount);

        // Sort entities by their zcodes
        {
            PHX_PROFILE_ZONE_SCOPED_N("SortBodiesByZCode_Sort");

            std::sort(
                std::execution::par,
                scratchBlock.SortedEntities.begin(),
                scratchBlock.SortedEntities.end(),
                [](const EntityBody& a, const EntityBody& b)
                {
                    return a.ZCode < b.ZCode;
                });
        }
    }
}

namespace detail
{
    void CalculateContactsTask(FeaturePhysicsScratchBlock& scratchBlock, DeltaTime dt, uint32 startIndex, uint32 count)
    {
        PHX_PROFILE_ZONE_SCOPED;

        for (uint32 i = 0; i < count; ++i)
        {
            Contact& contact = scratchBlock.Contacts[startIndex + i];
            ContactPair& contactPair = scratchBlock.ContactPairs[contact.ContactPair];
            auto& bodyCompA = *contactPair.BodyA;
            auto& bodyCompB = *contactPair.BodyB;
            auto& transformCompA = *contactPair.TransformA;
            auto& transformCompB = *contactPair.TransformB;

            Vec2 v;
            if (Vec2::Equals(transformCompA.Transform.Position, transformCompB.Transform.Position))
            {
                v = Vec2::RandUnitVector();
                auto correction = OneDivBy(bodyCompA.InvMass + bodyCompB.InvMass);
                transformCompA.Transform.Position -= v * correction * 0.01f;
                transformCompB.Transform.Position += v * correction * 0.01f;
            }

            v = transformCompB.Transform.Position - transformCompA.Transform.Position;
                
            Distance vLen = v.Length();
            Distance rr = bodyCompA.Radius + bodyCompB.Radius;

            constexpr Value baum = 0.3f;
            const Value slop = 0.01f * rr;
            Distance d = rr - vLen;
            Value bias = -baum * Max(0, d - slop) / dt;

            contact.Normal = v.Normalized();
            contact.Bias = bias;
            contact.EffMass = OneDivBy(bodyCompA.InvMass + bodyCompB.InvMass);
            contact.Impulse = 0;

            SetFlagRef(bodyCompA.Flags, EBodyFlags::Awake, true);
            SetFlagRef(bodyCompB.Flags, EBodyFlags::Awake, true);
        }
    }

    void PGSTask(FeaturePhysicsScratchBlock& scratchBlock, uint32 startIndex, uint32 count)
    {
        PHX_PROFILE_ZONE_SCOPED;

        for (uint32 i = 0; i < count; ++i)
        {
            Contact& contact = scratchBlock.Contacts[startIndex + i];
            ContactPair& contactPair = scratchBlock.ContactPairs[contact.ContactPair];
            
            // Relative velocity at contact
            Vec2 velA = Vec2::Zero;
            if (contactPair.BodyA)
            {
                velA = contactPair.BodyA->LinearVelocity;
            }

            Vec2 velB = Vec2::Zero;
            if (contactPair.BodyB)
            {
                velB = contactPair.BodyB->LinearVelocity;
            }

            Value relVel = Vec2::Dot(contact.Normal, velB - velA);
    
            // Compute corrective impulse
            Value lambda = -(relVel + contact.Bias) * contact.EffMass;
    
            // Accumulate and project (no negative normal impulses)
            Value oldImpulse = contact.Impulse;
            contact.Impulse = Max(oldImpulse + lambda, 0.0f);
            Value change = contact.Impulse - oldImpulse;
    
            // Apply impulse
            Vec2 p = contact.Normal * change;
            if (contactPair.BodyA && !HasAnyFlags(contactPair.BodyA->Flags, EBodyFlags::Static))
            {
                contactPair.BodyA->LinearVelocity -= p * contactPair.BodyA->InvMass;
            }
            if (contactPair.BodyB && !HasAnyFlags(contactPair.BodyB->Flags, EBodyFlags::Static))
            {
                contactPair.BodyB->LinearVelocity += p * contactPair.BodyB->InvMass;
            }
        }
    }

    void OverlapSeparationTask(FeaturePhysicsScratchBlock& scratchBlock, uint32 startIndex, uint32 count)
    {
        PHX_PROFILE_ZONE_SCOPED;

        for (uint32 i = 0; i < count; ++i)
        {
            EntityBody& entityBody = scratchBlock.SortedEntities[startIndex + i];
            auto bodyCompA = entityBody.BodyComponent;
            auto transformCompA = entityBody.TransformComponent;

            // Collide with lines
            {
                for (const CollisionLine& line : scratchBlock.CollisionLines)
                {
                    Vec2 v = Line2::VectorToLine(line.Line, transformCompA->Transform.Position);
                    Distance vLen = v.Length();
                    if (vLen != 0.0f && vLen < bodyCompA->Radius)
                    {
                        Vec2 n = -(v / vLen);
                        Vec2 d = n * (bodyCompA->Radius - vLen);
                        transformCompA->Transform.Position += d;
                        if (Vec2::Dot(bodyCompA->LinearVelocity, n) < 0)
                        {
                            bodyCompA->LinearVelocity = Vec2::Reflect(line.Line.GetDirection(), bodyCompA->LinearVelocity);
                        }
                        SetFlagRef(bodyCompA->Flags, EBodyFlags::Awake, true);
                    }
                }
            }
        }
    }

    void OverlapSeparationTask2(FeaturePhysicsScratchBlock& scratchBlock, uint32 startIndex, uint32 count)
    {
        PHX_PROFILE_ZONE_SCOPED;

        for (uint32 i = 0; i < count; ++i)
        {
            Contact& contact = scratchBlock.Contacts[startIndex + i];
            ContactPair& contactPair = scratchBlock.ContactPairs[contact.ContactPair];
        
            Vec2 v = contactPair.TransformB->Transform.Position - contactPair.TransformA->Transform.Position;
            Distance d = v.Length();
            Distance rr = contactPair.BodyA->Radius + contactPair.BodyB->Radius;
            Distance pen = rr - d;
            if (pen > 0.01)
            {
                Value correction = 0.01f * pen;
                contactPair.TransformA->Transform.Position -= contact.Normal * correction * contactPair.BodyA->InvMass / (contactPair.BodyA->InvMass + contactPair.BodyB->InvMass);
                contactPair.TransformB->Transform.Position += contact.Normal * correction * contactPair.BodyB->InvMass / (contactPair.BodyA->InvMass + contactPair.BodyB->InvMass);
            }
        }
    }
}

void PhysicsSystem::OnWorldUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    DeltaTime dt = args.DeltaTime;

    FeaturePhysicsDynamicBlock& dynamicBlock = world.GetBlockRef<FeaturePhysicsDynamicBlock>();
    FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();
    
    // Determine contacts
    {
        PHX_PROFILE_ZONE_SCOPED_N("CalculateContacts");

        CalculateContactPairsJob job;
        job.DeltaTime = dt;

        scratchBlock.ContactPairs.Reset();
        scratchBlock.ContactPairsCount = 0;

        FeatureECS::ScheduleParallel(world, job);

        FeatureECS::ExecutePendingJobs(world);

        scratchBlock.ContactPairs.SetSize(scratchBlock.ContactPairsCount);

        {
            PHX_PROFILE_ZONE_SCOPED_N("SortContactPairs");

            // Sort contact pairs
            std::sort(
                std::execution::par,
                scratchBlock.ContactPairs.begin(),
                scratchBlock.ContactPairs.end(),
                [](const ContactPair& a, const ContactPair& b)
                {
                    return a.Key < b.Key;
                });
        }

        {
            PHX_PROFILE_ZONE_SCOPED_N("ResolveContactPairs");
            
            scratchBlock.Contacts.Reset();

            uint64 currContactPairKey = 0;
            uint32 contacts = 0;
            for (uint32 i = 0; i < scratchBlock.ContactPairs.Num(); ++i)
            {
                const ContactPair& contactPair = scratchBlock.ContactPairs[i];

                if (contactPair.Key == currContactPairKey)
                    continue;

                currContactPairKey = contactPair.Key;

                Contact& contact = scratchBlock.Contacts[contacts++];
                contact.ContactPair = i;

                if (contacts == scratchBlock.Contacts.Capacity)
                {
                    break;
                }
            }

            scratchBlock.Contacts.SetSize(contacts);
        }

        ParallelRange(scratchBlock.Contacts.Num(), 128, [&](uint32 start, uint32 len)
        {
            detail::CalculateContactsTask(scratchBlock, dt, start, len);
        });
    }

    // Multi-pass solver
    if (1)
    {
        PHX_PROFILE_ZONE_SCOPED_N("PGS");

        for (uint32 i = 0; i < 4; ++i)
        {
            PHX_PROFILE_ZONE_SCOPED_N("PGSIter");
            PHX_PROFILE_ZONE_VALUE(i);

            ParallelRange(scratchBlock.Contacts.Num(), 128, [&](uint32 start, uint32 len)
            {
                detail::PGSTask(scratchBlock, start, len);
            });
        }
    }

    // Integrate velocities
    if (1)
    {
        PHX_PROFILE_ZONE_SCOPED_N("Integrate");

        IntegrateJob job;
        job.DeltaTime = dt;

        FeatureECS::ScheduleParallel(world, job);

        FeatureECS::ExecutePendingJobs(world);
    }

    // Multi-pass overlap separation
    if (1)
    {
        PHX_PROFILE_ZONE_SCOPED_N("MultiPassOverlapSeparation");

        for (uint32 i = 0; i < 4; ++i)
        {
            PHX_PROFILE_ZONE_SCOPED_N("MPOSIter");
            PHX_PROFILE_ZONE_VALUE(i);

            ParallelRange(scratchBlock.SortedEntities.Num(), 128, [&](uint32 start, uint32 len)
            {
                detail::OverlapSeparationTask(scratchBlock, start, len);
            });

            ParallelRange(scratchBlock.Contacts.Num(), 128, [&](uint32 start, uint32 len)
            {
                detail::OverlapSeparationTask2(scratchBlock, start, len);
            });
        }
    }
}

void PhysicsSystem::OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer)
{
    ISystem::OnDebugRender(world, state, renderer);

    const FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    for (const CollisionLine& collisionLine : scratchBlock.CollisionLines)
    {
        renderer.DrawLine(collisionLine.Line.Start, collisionLine.Line.End, Color(0, 255, 0));
    }

    if (bDebugDrawContacts)
    {
        for (const Contact& contact : scratchBlock.Contacts)
        {
            const ContactPair& contactPair = scratchBlock.ContactPairs[contact.ContactPair];
            Vec2 v = contact.Normal * contact.Bias;
            Vec2 s = contactPair.TransformA->Transform.Position;
            Vec2 e = s + v;
            renderer.DrawLine(s, e, Color(255, 255, 255));
        }
    }
}