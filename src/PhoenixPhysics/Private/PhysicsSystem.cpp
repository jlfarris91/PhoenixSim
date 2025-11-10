
#include "PhysicsSystem.h"

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

void PhysicsSystem::OnPreWorldUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    scratchBlock.NumIterations = 0;
    scratchBlock.NumCollisions = 0;
    scratchBlock.MaxQueryBodyCount = 0;
    scratchBlock.Contacts.Reset();
    scratchBlock.ContactSet.Reset();

    // Sort entity bodies by z-code (calculated by FeatureECS)
    {
        PHX_PROFILE_ZONE_SCOPED_N("SortBodiesByZCode");

        scratchBlock.SortedEntities.Reset();
        FeatureECS::Entities(world)
            .ForEachEntity(TFunction([&](const EntityComponentSpan<TransformComponent&, BodyComponent&>& span)
            {
                for (auto && [entityId, transformComp, bodyComp] : span)
                {
                    scratchBlock.SortedEntities.EmplaceBack(entityId, &transformComp, &bodyComp, transformComp.ZCode);
                }
            }));

        // Sort entities by their zcodes
        std::ranges::sort(
            scratchBlock.SortedEntities,
            [](const EntityBody& a, const EntityBody& b)
            {
                return a.ZCode < b.ZCode;
            });
    }
}

void PhysicsSystem::OnWorldUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    auto dt = args.DeltaTime;

    FeaturePhysicsDynamicBlock& dynamicBlock = world.GetBlockRef<FeaturePhysicsDynamicBlock>();
    FeaturePhysicsScratchBlock& scratchBlock = world.GetBlockRef<FeaturePhysicsScratchBlock>();

    TMortonCodeRangeArray ranges;
    
    // Determine contacts
    {
        PHX_PROFILE_ZONE_SCOPED_N("CalculateContacts");

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
            // {
            //     Distance radius = bodyCompA->Radius;
            //     for (const CollisionLine& line : scratchBlock.CollisionLines)
            //     {
            //         Vec2 v = Line2::VectorToLine(line.Line, transformCompA->Transform.Position);
            //         Distance vLen2 = v.Length();
            //         if (vLen2 < radius)
            //         {
            //             Contact& contact = scratchBlock.Contacts.AddDefaulted_GetRef();
            //             contact.TransformA = transformCompA;
            //             contact.BodyA = bodyCompA;
            //             contact.TransformB = nullptr;
            //             contact.BodyB = nullptr;
            //             contact.Normal = v.Normalized();
            //             contact.Bias = (v.Length() - bodyCompA->Radius) / dt;
            //             contact.EffMass = OneDivBy(bodyCompA->InvMass);
            //             contact.Impulse = 0;
            //
            //             SetFlagRef(bodyCompA->Flags, EBodyFlags::Awake, true);
            //         }
            //     }
            // }

            // Query for overlapping morton ranges
            TArray<const EntityBody*> overlappingBodies;
            {
                PHX_PROFILE_ZONE_SCOPED_N("OverlapQuery");

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
                    bool containsKey = scratchBlock.ContactSet.Contains(key);
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
                    scratchBlock.ContactSet.Insert(key);
                }
            }
        }
    }

    // Multi-pass solver
    if (1)
    {
        PHX_PROFILE_ZONE_SCOPED_N("PGS");

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
        PHX_PROFILE_ZONE_SCOPED_N("Integrate");

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
                if (dynamicBlock.bAllowSleep)
                {
                    bool isMoving = bodyComp->LinearVelocity.Length() > Distance(1E-1);
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
                }
                
                transformComp->Transform.Position += bodyComp->LinearVelocity * dt;

                bodyComp->LinearVelocity *= (1.0f - bodyComp->LinearDamping * dt);
            }
        }
    }

    // Multi-pass overlap separation
    if (1)
    {
        PHX_PROFILE_ZONE_SCOPED_N("MultiPassOverlapSeparation");

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

    if (bDebugDrawContacts)
    {
        for (const Contact& contact : scratchBlock.Contacts)
        {
            Vec2 v = contact.Normal * contact.Bias;
            Vec2 s = contact.TransformA->Transform.Position;
            Vec2 e = s + v;
            renderer.DrawLine(s, e, Color(255, 255, 255));
        }
    }
}