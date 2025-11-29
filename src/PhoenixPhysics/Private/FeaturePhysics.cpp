
#include "FeaturePhysics.h"

#include "BodyComponent.h"
#include "MortonCode.h"
#include "Profiling.h"
#include "Session.h"

using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;

FeaturePhysics::FeaturePhysics()
{
}

void FeaturePhysics::Initialize()
{
    PhysicsSystem = MakeShared<Physics::PhysicsSystem>();

    TSharedPtr<FeatureECS> featureECS = Session->GetFeatureSet()->GetFeature<FeatureECS>();
    featureECS->RegisterSystem(PhysicsSystem);
}

void FeaturePhysics::QueryEntitiesInRange(
    WorldConstRef world,
    const Vec2& pos,
    Distance range,
    TArray<EntityBody>& outEntities)
{
    PHX_PROFILE_ZONE_SCOPED;

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
            entityBody.BodyComponent->Force += dir.Normalized() * force * t;
        }
    }
}

void FeaturePhysics::AddForce(WorldRef world, EntityId entityId, const Vec2& force)
{
    BodyComponent* bodyComponent = FeatureECS::GetComponent<BodyComponent>(world, entityId);
    if (!bodyComponent)
    {
        return;
    }

    bodyComponent->Force += force;
}