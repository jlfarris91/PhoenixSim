
#include "FeaturePhysics.h"

#include <algorithm>

#include "Color.h"
#include "Debug.h"
#include "FeatureTrace.h"
#include "Flags.h"
#include "MortonCode.h"
#include "Session.h"

using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;

constexpr uint8 SLEEP_TIMER = 1;

void PhysicsSystem::OnPreUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    DeltaTime dt = args.DeltaTime;

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

    // Sort entity bodies by z-code (calculated by FeatureECS)
    {
        ScopedTrace trace(world, "SortBodiesByZCode"_n);
        scratchBlock.SortedEntities.Reset();
        for (auto && [entity, transformComp, bodyComp] : scratchBlock.EntityBodies)
        {
            scratchBlock.SortedEntities.EmplaceBack(entity->Id, transformComp, bodyComp, transformComp->ZCode);
        }

        // Sort entities by their zcodes
        std::ranges::sort(scratchBlock.SortedEntities,
            [](const EntityBody& a, const EntityBody& b)
            {
                return a.ZCode < b.ZCode;
            });
    }

    {
        ScopedTrace trace(world, "MoveBodies"_n);
        scratchBlock.MoveBodies.Refresh(world);

        for (auto && [entity, transComp, bodyComp, moveComp] : scratchBlock.MoveBodies)
        {
            Vec2 dir = scratchBlock.MapCenter - transComp->Transform.Position;
            if (dir.Length() < 0.1f)
            {
                continue;
            }

            dir = dir.Normalized();
            bodyComp->LinearVelocity += dir * moveComp->Speed * bodyComp->InvMass;

            Angle targetRot = dir.AsRadians();
            Angle deltaRot = AngleBetween(transComp->Transform.Rotation, targetRot);
            deltaRot = deltaRot * dt;
            transComp->Transform.Rotation += deltaRot;
        }
    }
}

void PhysicsSystem::OnUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    auto dt = args.DeltaTime;

    FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    TMortonCodeRangeArray ranges;

    size_t maxProbeLen = 0;

    Value margin = 0.5f;
    Distance mapMinDim = Min(scratchBlock.MapCenter.X, scratchBlock.MapCenter.Y);
    Vec2 offset = Vec2(mapMinDim, mapMinDim) * margin;
    
    Vec2 bl = Vec2(-offset.X, -offset.Y);
    Vec2 br = Vec2(offset.X, -offset.Y);
    Vec2 tl = Vec2(-offset.X, offset.Y);
    Vec2 tr = Vec2(offset.X, offset.Y);

    scratchBlock.Rotation += Deg2Rad(0.1f);

    Vec2 bl1 = bl.Rotate(scratchBlock.Rotation);
    Vec2 br1 = br.Rotate(scratchBlock.Rotation);
    Vec2 tl1 = tl.Rotate(scratchBlock.Rotation);
    Vec2 tr1 = tr.Rotate(scratchBlock.Rotation);

    bl = scratchBlock.MapCenter + bl1;
    br = scratchBlock.MapCenter + br1;
    tl = scratchBlock.MapCenter + tl1;
    tr = scratchBlock.MapCenter + tr1;
    
    // Determine contacts
    {
        ScopedTrace trace(world, "CalculateContacts"_n);
        for (const EntityBody& entityBodyA : scratchBlock.SortedEntities)
        {
            TransformComponent* transformCompA = entityBodyA.TransformComponent;
            BodyComponent* bodyCompA = entityBodyA.BodyComponent;

            if (!HasAnyFlags(bodyCompA->Flags, EBodyFlags::Awake))
            {
                continue;
            }

            Vec2 projectedPos = transformCompA->Transform.Position + bodyCompA->LinearVelocity * dt;

            // Collide with lines
            {
                Distance radius = bodyCompA->Radius;
                for (const CollisionLine& line : scratchBlock.CollisionLines)
                {
                    Vec2 v = Line2::VectorToLine(line.Line, transformCompA->Transform.Position);
                    Distance vLen2 = v.Length();
                    if (vLen2 < radius)
                    {
                        Contact& contact = scratchBlock.Contacts.AddDefaulted_GetRef();
                        contact.TransformA = transformCompA;
                        contact.BodyA = bodyCompA;
                        contact.TransformB = nullptr;
                        contact.BodyB = nullptr;
                        contact.Normal = v.Normalized();
                        contact.Bias = (v.Length() - bodyCompA->Radius) / dt;
                        contact.EffMass = OneDivBy(bodyCompA->InvMass);
                        contact.Impulse = 0;

                        SetFlagRef(bodyCompA->Flags, EBodyFlags::Awake, true);
                    }
                }
            }

            // Query for overlapping morton ranges
            TArray<const EntityBody*> overlappingBodies;
            {
                ScopedTrace trace2(world, "OverlapQuery"_n);

                MortonCodeAABB aabb = ToMortonCodeAABB(projectedPos, bodyCompA->Radius);
        
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
                scratchBlock.MaxQueryBodyCount = Max(scratchBlock.MaxQueryBodyCount, overlappingBodies.size() - 1);
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

                entityid_t loId = Min(entityBodyA.EntityId, entityBodyB->EntityId);
                entityid_t hiId = Max(entityBodyA.EntityId, entityBodyB->EntityId);
                uint64 key = hiId; key = key << 32 | loId;

                {
                    // ScopedTrace trace2(world, "ContactSetQuery"_n);
                    size_t probeLen = 0;
                    bool containsKey = scratchBlock.ContactSet.Contains(key, &probeLen);
                    maxProbeLen = Max(maxProbeLen, probeLen);
                    if (containsKey)
                    {
                        continue;
                    }
                }
            
                ++scratchBlock.NumIterations;

                Vec2 v;
                if (Vec2::Equals(transformCompA->Transform.Position, transformCompB->Transform.Position))
                {
                    v = Vec2::RandUnitVector();
                    auto correction = OneDivBy(bodyCompA->InvMass + bodyCompB->InvMass);
                    transformCompA->Transform.Position -= v * correction * 0.01f;
                    transformCompB->Transform.Position += v * correction * 0.01f;
                }

                v = transformCompB->Transform.Position - transformCompA->Transform.Position;
            
                Distance vLen = v.Length();
                Distance rr = bodyCompA->Radius + bodyCompB->Radius;
                if (vLen > rr)
                {
                    continue;
                }

                const Value baum = 0.3f;
                const Value slop = 0.01f * rr;
                Distance d = rr - vLen;
                Value bias = -baum * Max(0, d - slop) / dt;

                Contact& contact = scratchBlock.Contacts.AddDefaulted_GetRef();
                contact.TransformA = transformCompA;
                contact.BodyA = bodyCompA;
                contact.TransformB = transformCompB;
                contact.BodyB = bodyCompB;
                contact.Normal = v.Normalized();
                contact.Bias = bias;
                contact.EffMass = OneDivBy(bodyCompA->InvMass + bodyCompB->InvMass);
                contact.Impulse = 0;

                SetFlagRef(bodyCompA->Flags, EBodyFlags::Awake, true);
                SetFlagRef(bodyCompB->Flags, EBodyFlags::Awake, true);

                ++scratchBlock.NumCollisions;

                {
                    // ScopedTrace trace2(world, "ContactSetQuery"_n);
                    size_t probeLen = 0;
                    scratchBlock.ContactSet.Insert(key, &probeLen);
                    maxProbeLen = Max(maxProbeLen, probeLen);
                }
            }
        }
    }

    FeatureTrace::PushTrace(world, "ContactSetSize"_n, {}, ETraceFlags::Counter, (int32)scratchBlock.ContactSet.Num());
    FeatureTrace::PushTrace(world, "MaxProbeLen"_n, {}, ETraceFlags::Counter, (int32)maxProbeLen);

    // Multi-pass solver
    if (1)
    {        
        ScopedTrace trace(world, "PGS"_n);
        for (uint32 iter = 0; iter < 4; ++iter)
        {
            for (Contact& contact : scratchBlock.Contacts)
            {
                // Relative velocity at contact
                Vec2 velA = Vec2::Zero;
                if (contact.BodyA)
                {
                    velA = contact.BodyA->LinearVelocity;
                }

                Vec2 velB = Vec2::Zero;
                if (contact.BodyB)
                {
                    velB = contact.BodyB->LinearVelocity;
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
                if (contact.BodyA && !HasAnyFlags(contact.BodyA->Flags, EBodyFlags::Static))
                {
                    contact.BodyA->LinearVelocity -= p * contact.BodyA->InvMass;
                }
                if (contact.BodyB && !HasAnyFlags(contact.BodyB->Flags, EBodyFlags::Static))
                {
                    contact.BodyB->LinearVelocity += p * contact.BodyB->InvMass;
                }
            }   
        }
    }

    // Integrate velocities
    if (1)
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
                bool isMoving = bodyComp->LinearVelocity.Length() > Distance::Epsilon;
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

    // Multi-pass overlap separation
    if (1)
    {
        for (uint32 i = 0; i < 4; ++i)
        {
            for (auto entityBody : scratchBlock.SortedEntities)
            {
                auto bodyCompA = entityBody.BodyComponent;
                auto transformCompA = entityBody.TransformComponent;

                // Collide with lines
                {
                    Distance radius = bodyCompA->Radius;// * bodyCompA->Radius;
                    for (const CollisionLine& line : scratchBlock.CollisionLines)
                    {
                        Vec2 v = Line2::VectorToLine(line.Line, transformCompA->Transform.Position);
                        Distance vLen = v.Length();
                        if (vLen != 0.0f && vLen < radius)
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

            for (const Contact& contact : scratchBlock.Contacts)
            {
                if (contact.BodyA == nullptr || contact.BodyB == nullptr)
                {
                    continue;
                }
        
                Vec2 v = contact.TransformB->Transform.Position - contact.TransformA->Transform.Position;
                Distance d = v.Length();
                Distance rr = contact.BodyA->Radius + contact.BodyB->Radius;
                Distance pen = rr - d;
                if (pen > 0.01)
                {
                    Value correction = 0.01f * pen;
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
}

void PhysicsSystem::OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer)
{
    ISystem::OnDebugRender(world, state, renderer);

    const FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    for (const CollisionLine& collisionLine : scratchBlock.CollisionLines)
    {
        renderer.DrawLine(collisionLine.Line.Start, collisionLine.Line.End, Color(0, 255, 0));
    }

    for (const Contact& contact : scratchBlock.Contacts)
    {
        Vec2 v = contact.Normal * contact.Bias;
        Vec2 s = contact.TransformA->Transform.Position;
        Vec2 e = s + v;
        renderer.DrawLine(s, e, Color(255, 255, 255));
    }
}

FeaturePhysics::FeaturePhysics()
{
    FeatureDefinition.Name = StaticName;
    FeatureDefinition.RegisterBlock<FeaturePhysicsScratchBlock>();
    FeatureDefinition.RegisterChannel(WorldChannels::HandleAction);
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

        TArray<EntityBody> outEntities;
        QueryEntitiesInRange(world, pos, range, outEntities);

        for (const EntityBody& entityBody : outEntities)
        {
            const Vec2& entityPos = entityBody.TransformComponent->Transform.Position;
            if (Vec2::Distance(pos, entityPos) < range)
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

    if (action.Action.Verb == "set_map_center"_n)
    {
        FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();
        scratchBlock.MapCenter = { action.Action.Data[0].Distance, action.Action.Data[1].Distance };
    }
}

void FeaturePhysics::QueryEntitiesInRange(
    WorldConstRef world,
    const Vec2& pos,
    Distance range,
    TArray<EntityBody>& outEntities)
{
    const FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    TMortonCodeRangeArray ranges;
    
    // Query for overlapping morton ranges
    {
        MortonCodeAABB aabb = ToMortonCodeAABB(pos, range);
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
    // Distance rangeSq = range * range;

    TArray<EntityBody> outEntities;
    QueryEntitiesInRange(world, pos, range, outEntities);

    for (const EntityBody& entityBody : outEntities)
    {
        const Vec2& entityPos = entityBody.TransformComponent->Transform.Position;
        Vec2 dir = entityPos - pos;
        Distance dist = dir.Length();
        if (dist < range)
        {
            Value t = 1.0f - dist / range;
            Value f = force / entityBody.BodyComponent->InvMass;
            entityBody.BodyComponent->LinearVelocity += dir.Normalized() * f * t;
        }
    }
}
