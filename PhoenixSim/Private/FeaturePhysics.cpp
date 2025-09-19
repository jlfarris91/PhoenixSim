
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
    Value dt = 1.0f / args.StepHz;

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

    {
        ScopedTrace trace(world, "MoveBodies"_n);
        scratchBlock.MoveBodies.Refresh(world);

        for (auto && [entity, transComp, bodyComp, moveComp] : scratchBlock.MoveBodies)
        {
            Vec2 dir = scratchBlock.MapCenter - transComp->Transform.Position;
            if (dir.LengthSq() < 0.1f * 0.1f)
            {
                continue;
            }

            dir = dir.Normalized();
            bodyComp->LinearVelocity += dir * moveComp->Speed / bodyComp->InvMass;

            Degrees targetRot = dir.AsDegrees();
            Degrees deltaRot = targetRot - transComp->Transform.Rotation;
            transComp->Transform.Rotation += deltaRot * dt;
        }
    }
}

void PhysicsSystem::OnUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    Value dt = 1.0f / args.StepHz;

    FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    std::vector<TTuple<uint64, uint64>> ranges;

    size_t maxProbeLen = 0;

    Value margin = 0.5f;
    Distance mapMinDim = min(scratchBlock.MapCenter.X, scratchBlock.MapCenter.Y);
    Vec2 offset = Vec2(mapMinDim) * margin;
    
    Vec2 bl = Vec2(-offset.X, -offset.Y);
    Vec2 br = Vec2(offset.X, -offset.Y);
    Vec2 tl = Vec2(-offset.X, offset.Y);
    Vec2 tr = Vec2(offset.X, offset.Y);

    scratchBlock.Rotation += 0.1f;

    bl = scratchBlock.MapCenter + bl.Rotate(scratchBlock.Rotation);
    br = scratchBlock.MapCenter + br.Rotate(scratchBlock.Rotation);
    tl = scratchBlock.MapCenter + tl.Rotate(scratchBlock.Rotation);
    tr = scratchBlock.MapCenter + tr.Rotate(scratchBlock.Rotation);

    // scratchBlock.Rotation += 0.1f;

    scratchBlock.CollisionLines.Reset();
    scratchBlock.CollisionLines.EmplaceBack(Line2(bl, br));
    scratchBlock.CollisionLines.EmplaceBack(Line2(tr, br));
    scratchBlock.CollisionLines.EmplaceBack(Line2(tr, tl));
    scratchBlock.CollisionLines.EmplaceBack(Line2(bl, tl));

    Vec2 a1 = Vec2(offset.X, offset.Y) * 0.25f;
    Vec2 a2 = Vec2(offset.X, offset.Y) * -0.25f;
    a1 = scratchBlock.MapCenter + a1.Rotate(scratchBlock.Rotation);
    a2 = scratchBlock.MapCenter + a2.Rotate(scratchBlock.Rotation);
    
    scratchBlock.CollisionLines.EmplaceBack(Line2(a1, a2));

    a1 = Vec2(offset.X, offset.Y) * 0.25f;
    a2 = Vec2(offset.X, offset.Y) * -0.25f;
    a1 = scratchBlock.MapCenter + a1.Rotate(scratchBlock.Rotation + 90.0f);
    a2 = scratchBlock.MapCenter + a2.Rotate(scratchBlock.Rotation + 90.0f);
    
    scratchBlock.CollisionLines.EmplaceBack(Line2(a1, a2));
    
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
                Distance radiusSq = bodyCompA->Radius * bodyCompA->Radius;
                for (const CollisionLine& line : scratchBlock.CollisionLines)
                {
                    Vec2 v = Line2::VectorToLine(line.Line, transformCompA->Transform.Position);
                    if (v.LengthSq() < radiusSq)
                    {
                        Vec2 n = v.Normalized();
                        Value s = Vec2::Dot(bodyCompA->LinearVelocity * dt, n);
                        
                        Contact& contact = scratchBlock.Contacts.AddDefaulted_GetRef();
                        contact.TransformA = transformCompA;
                        contact.BodyA = bodyCompA;
                        contact.TransformB = nullptr;
                        contact.BodyB = nullptr;
                        contact.Normal = n;
                        contact.Bias = (v.Length() - bodyCompA->Radius) / dt;
                        contact.EffMass = 1.0f / bodyCompA->InvMass;
                        contact.Impulse = 0;

                        SetFlagRef(bodyCompA->Flags, EBodyFlags::Awake, true);
                    }
                }
            }

            // Query for overlapping morton ranges
            TArray<const EntityBody*> overlappingBodies;
            {
                ScopedTrace trace2(world, "OverlapQuery"_n);

                Distance radius = bodyCompA->Radius;
                uint32 lox = static_cast<uint32>(projectedPos.X - radius);
                uint32 hix = static_cast<uint32>(projectedPos.X + radius);
                uint32 loy = static_cast<uint32>(projectedPos.Y - radius);
                uint32 hiy = static_cast<uint32>(projectedPos.Y + radius);

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

                entityid_t loId = min(entityBodyA.EntityId, entityBodyB->EntityId);
                entityid_t hiId = max(entityBodyA.EntityId, entityBodyB->EntityId);
                uint64 key = hiId; key = key << 32 | loId;

                {
                    // ScopedTrace trace2(world, "ContactSetQuery"_n);
                    size_t probeLen = 0;
                    bool containsKey = scratchBlock.ContactSet.Contains(key, &probeLen);
                    maxProbeLen = max(maxProbeLen, probeLen);
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
                    float correction = 1.0f / (bodyCompA->InvMass + bodyCompB->InvMass);
                    transformCompA->Transform.Position -= v * correction * 0.01f;
                    transformCompB->Transform.Position += v * correction * 0.01f;
                }

                v = transformCompB->Transform.Position - transformCompA->Transform.Position;
            
                float vLen = v.Length();
                float rr = bodyCompA->Radius + bodyCompB->Radius;
                if (vLen > rr)
                {
                    continue;
                }

                Value invMassComb = 0;
                if (!HasAnyFlags(bodyCompA->Flags, EBodyFlags::Static))
                {
                    invMassComb += bodyCompA->InvMass;
                }
                if (!HasAnyFlags(bodyCompB->Flags, EBodyFlags::Static))
                {
                    invMassComb += bodyCompB->InvMass;
                }
                
                float effMass = 1.0f;
                if (invMassComb == 0.0f)
                {
                    effMass /= invMassComb;
                }
            
                const float baum = 0.3f;
                const float slop = 0.01f * rr;
                float d = rr - vLen;
                float bias = -baum * max(0, d - slop) / dt;

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

                {
                    // ScopedTrace trace2(world, "ContactSetQuery"_n);
                    size_t probeLen = 0;
                    scratchBlock.ContactSet.Insert(key, &probeLen);
                    maxProbeLen = max(maxProbeLen, probeLen);
                }
            }
        }
    }

    FeatureTrace::PushTrace(world, "ContactSetSize"_n, {}, ETraceFlags::Counter, scratchBlock.ContactSet.Num());
    FeatureTrace::PushTrace(world, "MaxProbeLen"_n, {}, ETraceFlags::Counter, maxProbeLen);

    // Multi-pass solver
    {        
        ScopedTrace trace(world, "PGS"_n);
        for (uint32 iter = 0; iter < 100; ++iter)
        {
            for (Contact& contact : scratchBlock.Contacts)
            {
                // Relative velocity at contact
                Vec2 velA = 0.0f;
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
                contact.Impulse = max(oldImpulse + lambda, 0.0f);
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

                //if (bodyComp->Movement != EBodyMovement::Moving)
                {
                    bodyComp->LinearVelocity *= (1.0f - bodyComp->LinearDamping * dt);
                }
            }
        }
    }

    // Multi-pass overlap separation
    for (uint32 i = 0; i < 4; ++i)
    {
        for (auto entityBody : scratchBlock.SortedEntities)
        {
            auto bodyCompA = entityBody.BodyComponent;
            auto transformCompA = entityBody.TransformComponent;

            // Collide with lines
            {
                Distance radiusSq = bodyCompA->Radius * bodyCompA->Radius;
                for (const CollisionLine& line : scratchBlock.CollisionLines)
                {
                    Vec2 v = Line2::VectorToLine(line.Line, transformCompA->Transform.Position);
                    Distance vLenSq = v.LengthSq();
                    if (vLenSq != 0.0f && vLenSq < radiusSq)
                    {
                        Distance vLen = sqrt(vLenSq);
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
            float d = v.Length();
            float rr = contact.BodyA->Radius + contact.BodyB->Radius;
            float pen = rr - d;
            if (pen > 0.01)
            {
                float correction = 0.01f * pen;
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
