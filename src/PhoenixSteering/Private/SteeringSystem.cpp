
#include "SteeringSystem.h"

#include "BodyComponent.h"
#include "Debug.h"
#include "FeatureECS.h"
#include "FeatureNavigation.h"
#include "FeaturePhysics.h"
#include "Flags.h"
#include "MortonCode.h"
#include "SteeringComponent.h"
#include "SystemJob.h"
#include "TransformComponent.h"

using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;
using namespace Phoenix::Steering;

namespace SteeringDetail
{
    struct InitJob : IBufferJob<SteeringComponent&>
    {
        void Execute(const EntityComponentSpan<SteeringComponent&>& span) override
        {
            PHX_PROFILE_ZONE_SCOPED_N("InitJob");

            for (auto && [entityId, index, steeringComp] : span)
            {
                steeringComp.SteeringVector = Vec2::Zero;
            }
        }
    };

    struct SeekJob : IBufferJob<TransformComponent&, SteeringComponent&, SeekComponent&>
    {
        DeltaTime DeltaTime;

        void Execute(const EntityComponentSpan<TransformComponent&, SteeringComponent&, SeekComponent&>& span) override
        {
            PHX_PROFILE_ZONE_SCOPED_N("SeekJob");

            WorldRef world = *World;

            for (auto && [entityId, index, transformComp, steeringComp, seekComp] : span)
            {
                Vec2 targetPos = seekComp.TargetPos;

                if (seekComp.TargetEntity != EntityId::Invalid)
                {
                    if (const TransformComponent* targetTransformComp = FeatureECS::GetComponent<TransformComponent>(world, seekComp.TargetEntity))
                    {
                        targetPos = targetTransformComp->Transform.Position;
                    }
                }

                const Transform2D& transform = transformComp.Transform;
                const Vec2& currPos = transform.Position;
                Vec2 steeringVel;

                // Pathfinding::PathResult result = Pathfinding::FeatureNavigation::PathTo(world, currPos, targetPos, bodyComp.Radius);
                // if (result.PathFound)
                // {
                //     targetPos = result.NextPoint;
                // }

                if (HasAnyFlags(seekComp.Flags, ESeekFlags::Arrive) && seekComp.SlowingDistance > 0)
                {
                    Vec2 targetOffset = targetPos - currPos;
                    Distance distance = targetOffset.Length();
                    if (distance > 0.0)
                    {
                        Speed rampedSpeed = seekComp.MaxSpeed * (distance / seekComp.SlowingDistance);
                        Speed clippedSpeed = Min(rampedSpeed, seekComp.MaxSpeed);
                        Vec2 desiredVel = (clippedSpeed / distance) * targetOffset;
                        steeringVel = desiredVel - steeringVel;
                    }
                }
                else
                {
                    Vec2 desiredVel = (targetPos - currPos).Normalized() * seekComp.MaxSpeed;
                    steeringVel = desiredVel - steeringVel;
                }

                if (HasAnyFlags(seekComp.Flags, ESeekFlags::Flee))
                {   
                    steeringVel *= -1;
                }

                steeringComp.SteeringVector += steeringVel;
            }
        }
    };

    struct SeparationJob : IBufferJob<const TransformComponent&, const BodyComponent&, SteeringComponent&>
    {
        DeltaTime DeltaTime;
        bool bMoveTowardsGoal;
        Distance DensityScalar;
        Distance DensityRadiusScalar;
        Distance AvoidanceScalar;
        Distance AvoidanceRadiusScalar;

        void Execute(const EntityComponentSpan<const TransformComponent&, const BodyComponent&, SteeringComponent&>& span) override
        {
            PHX_PROFILE_ZONE_SCOPED_N("SeparationJob");

            FeaturePhysicsScratchBlock& physicsScratchBlock = World->GetBlockRef<FeaturePhysicsScratchBlock>();

            TMortonCodeRangeArray ranges;

            const EntityBody* neighbors[64];
            uint32 neighborsCount = 0;

            for (auto && [
                entityId,
                index,
                transformComp,
                bodyComp,
                steeringComp] : span)
            {
                Vec2 goal;
                Vec2 avoid;
                Vec2 density;
                const Value T = 0.2;

                if (bMoveTowardsGoal)
                {
                    goal = (Vec2::Zero - transformComp.Transform.Position).Normalized() * steeringComp.MaxSpeed;
                }

                // Query for overlapping morton ranges
                {
                    PHX_PROFILE_ZONE_SCOPED_N("OverlapQuery");

                    MortonCodeAABB aabb = ToMortonCodeAABB(transformComp.Transform.Position, steeringComp.AvoidanceRadius);

                    ranges.clear();
                    MortonCodeQuery(aabb, ranges);

                    neighborsCount = 0;
                    ForEachInMortonCodeRanges<EntityBody, &EntityBody::ZCode>(
                        physicsScratchBlock.SortedEntities,
                        ranges,
                        [&](const EntityBody& eb)
                        {
                            if (eb.EntityId == entityId)
                            {
                                return false;
                            }

                            if ((bodyComp.CollisionMask & eb.BodyComponent->CollisionMask) == 0)
                            {
                                return false;
                            }
                
                            neighbors[neighborsCount++] = &eb;
                            return neighborsCount == _countof(neighbors);
                        });
                }

                for (uint32 i = 0; i < neighborsCount; ++i)
                {
                    const EntityBody* entityBodyB = neighbors[i];
                    TransformComponent& transformCompB = *entityBodyB->TransformComponent;
                    BodyComponent& bodyCompB = *entityBodyB->BodyComponent;

                    Vec2 offset = transformComp.Transform.Position - transformCompB.Transform.Position;
                    Distance dist = offset.Length();
                    if (dist == 0.0)
                    {
                        offset = Vec2((1 + rand() % 99) / 100.0, (1 + rand() % 99) / 100.0);
                        offset = offset.Normalized();
                        dist = offset.Length();
                    }

                    if (dist == 0.0)
                    {
                        continue;
                    }

                    // Density
                    Distance target = bodyComp.Radius * DensityRadiusScalar;
                    if (dist < target)
                    {
                        Vec2 n = offset / dist;
                        auto k = (target - dist) / target;
                        density += n * k * DensityScalar;
                    }

                    // Avoid
                    Vec2 relVel = bodyComp.LinearVelocity - bodyCompB.LinearVelocity;
                    Vec2 d = offset + relVel * T;
                    Distance dLen = d.Length();
                    Distance r = (bodyComp.Radius + bodyCompB.Radius) * AvoidanceRadiusScalar;

                    if (dLen != 0 && dLen < r)
                    {
                        Vec2 correction = (d / dLen) * (r - dist);
                        Vec2 avoidVel = correction / T;
                        Vec2::EPerpendicularDir perpDir = static_cast<Vec2::EPerpendicularDir>((entityId * 2654435761u) & 1u);
                        Vec2 tangent = Vec2::Perpendicular(d, perpDir).Normalized();
                        Vec2 lateral = tangent * Vec2::Dot(relVel, tangent);
                        avoid += avoidVel * 0.4 + lateral * 0.6;
                    }
                }

                avoid *= AvoidanceScalar;

                steeringComp.GoalVector = goal;
                steeringComp.AvoidVector = avoid;
                steeringComp.DensityVector = density;
                steeringComp.SteeringVector = goal + avoid + density;
            }
        }
    };

    struct AvoidanceJob : IBufferJob<const TransformComponent&, const BodyComponent&, SteeringComponent&>
    {
        DeltaTime DeltaTime;

        void Execute(const EntityComponentSpan<const TransformComponent&, const BodyComponent&, SteeringComponent&>& span) override
        {
            PHX_PROFILE_ZONE_SCOPED_N("AvoidanceJob");

            FeaturePhysicsScratchBlock& physicsScratchBlock = World->GetBlockRef<FeaturePhysicsScratchBlock>();

            TMortonCodeRangeArray ranges;

            const EntityBody* overlappingBodies[16];
            uint32 overlappingBodiesCount = 0;

            for (auto && [
                entityId,
                index,
                transformComp,
                bodyComp,
                steeringComp] : span)
            {
                Vec2 avoidanceVec;
                Vec2 desiredVel = bodyComp.Force + steeringComp.SteeringVector;

                // Query for overlapping morton ranges
                {
                    PHX_PROFILE_ZONE_SCOPED_N("OverlapQuery");

                    MortonCodeAABB aabb = ToMortonCodeAABB(transformComp.Transform.Position, steeringComp.AvoidanceRadius * 2);

                    ranges.clear();
                    MortonCodeQuery(aabb, ranges);

                    overlappingBodiesCount = 0;
                    ForEachInMortonCodeRanges<EntityBody, &EntityBody::ZCode>(
                        physicsScratchBlock.SortedEntities,
                        ranges,
                        [&](const EntityBody& eb)
                        {
                            if (eb.EntityId == entityId)
                            {
                                return false;
                            }
                
                            if ((bodyComp.CollisionMask & eb.BodyComponent->CollisionMask) == 0)
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

                    Vec2 offset = transformCompB.Transform.Position - transformComp.Transform.Position;
                    Distance dist = offset.Length();
                    Distance rr = (bodyComp.Radius + bodyCompB.Radius) * 2;

                    if (dist != 0.0 && dist < steeringComp.AvoidanceRadius)
                    {
                        Vec2 n = offset / dist;
                        Value inward = Vec2::Dot(desiredVel, n);
                        if (inward > 0)
                        {
                            Value weight = Clamp<Value>((rr - dist) / rr, 0, 1);
                            avoidanceVec -= n * inward * weight;
                        }
                    }
                }

                steeringComp.SteeringVector += avoidanceVec;
            }
        }
    };

    struct WanderJob : IBufferJob<TransformComponent&, BodyComponent&, WanderComponent&>
    {
        void Execute(const EntityComponentSpan<TransformComponent&, BodyComponent&, WanderComponent&>& span) override
        {
            PHX_PROFILE_ZONE_SCOPED_N("WanderJob");

            for (auto && [entityId, index, transformComp, bodyComp, wander] : span)
            {
                auto a = ((rand() % RAND_MAX) / (double)RAND_MAX) * Angle::D * 2 - Angle::D;
                auto angleDelta = Angle(Q32(a)) * 0.2;
                wander.WanderAngle += angleDelta;
                Vec2 desiredVel = Vec2::FromPolar(wander.WanderAngle, wander.WanderRadius).Normalized() * wander.MaxSpeed;
                bodyComp.LinearVelocity = desiredVel;

                Angle desiredAngle = bodyComp.LinearVelocity.AsRadians();
                transformComp.Transform.Rotation = desiredAngle;
            }
        }
    };

    struct IntegrateJob : IBufferJob<TransformComponent&, BodyComponent&, SteeringComponent&>
    {
        void Execute(const EntityComponentSpan<TransformComponent&, BodyComponent&, SteeringComponent&>& span) override
        {
            PHX_PROFILE_ZONE_SCOPED_N("IntegrateJob");

            for (auto && [entityId, index, transformComp, bodyComp, steeringComp] : span)
            {
                Distance steerMag = steeringComp.SteeringVector.Length();
                if (steerMag != 0.0)
                {
                    TInvFixed2<Distance> invSteerMag = OneDivBy(steerMag);
                    Distance clampedMag = Clamp(steerMag, -steeringComp.MaxSpeed, steeringComp.MaxSpeed);
                    steeringComp.SteeringVector = (steeringComp.SteeringVector * invSteerMag) * clampedMag;
                }

                bodyComp.Force += steeringComp.SteeringVector;

                //Angle desiredAngle = bodyComp.LinearVelocity.AsRadians();
                //transformComp.Transform.Rotation = desiredAngle;
            }
        }
    };
}

void SteeringSystem::OnPreWorldUpdate(WorldRef world, const ECS::SystemUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    SteeringDetail::InitJob initJob;
    FeatureECS::ScheduleParallel(world, initJob);
}

void SteeringSystem::OnWorldUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    // SteeringDetail::SeekJob seekJob;
    // seekJob.DeltaTime = args.DeltaTime;
    // FeatureECS::ScheduleParallel(world, seekJob);

    SteeringDetail::WanderJob wanderJob;
    FeatureECS::ScheduleParallel(world, wanderJob);

    SteeringDetail::SeparationJob avoidanceJob;
    avoidanceJob.DeltaTime = args.DeltaTime;
    avoidanceJob.bMoveTowardsGoal = MoveTowardsGoal;
    avoidanceJob.DensityScalar = DensityScalar;
    avoidanceJob.DensityRadiusScalar = DensityRadiusScalar;
    avoidanceJob.AvoidanceScalar = AvoidanceScalar;
    avoidanceJob.AvoidanceRadiusScalar = AvoidanceRadiusScalar;
    FeatureECS::ScheduleParallel(world, avoidanceJob);

    SteeringDetail::IntegrateJob integrateJob;
    FeatureECS::ScheduleParallel(world, integrateJob);
}

void SteeringSystem::OnDebugRender(
    WorldConstRef world,
    const IDebugState& state,
    IDebugRenderer& renderer)
{
    ISystem::OnDebugRender(world, state, renderer);

    // EntityQueryBuilder builder;
    // builder.RequireAllComponents<TransformComponent, BodyComponent, SteeringComponent>();
    // auto query = builder.GetQuery();
    //
    // FeatureECS::ForEachEntity(world, query, TFunction([&](const EntityComponentSpan<const TransformComponent&, const BodyComponent&, const SteeringComponent&>& span)
    // {
    //     for (auto && [entity, index, transformComp, bodyComp, steeringComp] : span)
    //     {
    //         Vec2 start = transformComp.Transform.Position;
    //
    //         Vec2 dir = steeringComp.SteeringVector * bodyComp.InvMass * OneDivBy(Distance(60.0));
    //         renderer.DrawRay(start, dir, Color::Blue);
    //
    //         dir = steeringComp.GoalVector * bodyComp.InvMass * OneDivBy(Distance(60.0));
    //         renderer.DrawRay(start, dir, Color::Green);
    //
    //         dir = steeringComp.AvoidVector * bodyComp.InvMass * OneDivBy(Distance(60.0));
    //         renderer.DrawRay(start, dir, Color::Red);
    //
    //         dir = steeringComp.DensityVector * bodyComp.InvMass * OneDivBy(Distance(60.0));
    //         renderer.DrawRay(start, dir, Color::Yellow);
    //     }
    // }));
}
