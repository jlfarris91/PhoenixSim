
#include "SteeringSystem.h"

#include "BodyComponent.h"
#include "FeatureECS.h"
#include "Flags.h"
#include "SteeringComponent.h"
#include "SystemJob.h"
#include "TransformComponent.h"

using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;
using namespace Phoenix::Steering;

namespace SteeringDetail
{
    struct SeekJob : IBufferJob<TransformComponent&, BodyComponent&, SeekComponent&>
    {
        void Execute(const EntityComponentSpan<TransformComponent&, BodyComponent&, SeekComponent&>& span) override
        {
            PHX_PROFILE_ZONE_SCOPED_N("SeekJob");

            WorldRef world = *World;

            for (auto && [entityId, index, transformComp, bodyComp, seek] : span)
            {
                Vec2 targetPos = seek.TargetPos;

                if (seek.TargetEntity != EntityId::Invalid)
                {
                    if (const TransformComponent* targetTransformComp = FeatureECS::GetComponent<TransformComponent>(world, seek.TargetEntity))
                    {
                        targetPos = targetTransformComp->Transform.Position;
                    }
                }

                const Transform2D& transform = transformComp.Transform;
                const Vec2& currPos = transform.Position;
                Vec2 steeringVel;

                if (HasAnyFlags(seek.Flags, ESeekFlags::Arrive) && seek.SlowingDistance > 0)
                {
                    Vec2 targetOffset = targetPos - currPos;
                    Distance distance = targetOffset.Length();
                    if (distance > 0.0)
                    {
                        Speed rampedSpeed = seek.MaxSpeed * (distance / seek.SlowingDistance);
                        Speed clippedSpeed = Min(rampedSpeed, seek.MaxSpeed);
                        Vec2 desiredVel = (clippedSpeed / distance) * targetOffset;
                        steeringVel = desiredVel - steeringVel;
                    }
                }
                else
                {
                    Vec2 desiredVel = (targetPos - currPos).Normalized() * seek.MaxSpeed;
                    steeringVel = desiredVel - steeringVel;
                }
                
                if (HasAnyFlags(seek.Flags, ESeekFlags::Flee))
                {   
                    steeringVel *= -1;
                }

                bodyComp.LinearVelocity = steeringVel;

                Angle desiredAngle = bodyComp.LinearVelocity.AsRadians();
                transformComp.Transform.Rotation = desiredAngle;
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
}

void SteeringSystem::OnWorldUpdate(WorldRef world, const SystemUpdateArgs& args)
{
    ISystem::OnWorldUpdate(world, args);

    PHX_PROFILE_ZONE_SCOPED;

    SteeringDetail::SeekJob job;
    FeatureECS::ScheduleParallel(world, job);

    SteeringDetail::WanderJob wanderJob;
    FeatureECS::ScheduleParallel(world, wanderJob);
}

void SteeringSystem::OnDebugRender(
    WorldConstRef world,
    const IDebugState& state,
    IDebugRenderer& renderer)
{
    ISystem::OnDebugRender(world, state, renderer);
}
