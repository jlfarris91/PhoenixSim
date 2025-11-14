
#include "FeaturePhysics.h"

#include "BodyComponent.h"
#include "Flags.h"
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

bool FeaturePhysics::OnHandleWorldAction(WorldRef world, const FeatureActionArgs& action)
{
    IFeature::OnHandleWorldAction(world, action);

    // TODO (jfarris): move this to script
    if (action.Action.Verb == "spawn_entity"_n)
    {   
        for (uint32 i = 0; i < action.Action.Data[4].UInt32; ++i)
        {
            EntityId entityId = FeatureECS::AcquireEntity(world, action.Action.Data[0].Name);
            if (entityId == EntityId::Invalid)
                break;

            TransformComponent* transformComp = FeatureECS::GetComponent<TransformComponent>(world, entityId);
            PHX_ASSERT(transformComp);
            transformComp->Transform.Position.X = action.Action.Data[1].Distance;
            transformComp->Transform.Position.Y = action.Action.Data[2].Distance;
            transformComp->Transform.Rotation = action.Action.Data[3].Degrees;

            BodyComponent* bodyComp = FeatureECS::GetComponent<BodyComponent>(world, entityId);
            bodyComp->CollisionMask = 1;
            bodyComp->Radius = 0.6; // Lancer :)
            bodyComp->InvMass = OneDivBy<Value>(1.0f);
            bodyComp->LinearDamping = 5.f;
            SetFlagRef(bodyComp->Flags, EBodyFlags::Awake, true);
        }

        return true;
    }

    if (action.Action.Verb == "push_entities_in_range"_n)
    {
        Vec2 pos = { action.Action.Data[0].Distance, action.Action.Data[1].Distance };
        Distance range = action.Action.Data[2].Distance;
        Value force = action.Action.Data[3].Distance;
        AddExplosionForceToEntitiesInRange(world, pos, range, force);

        return true;
    }

    if (action.Action.Verb == "set_allow_sleep"_n)
    {
        bool allowSleep = action.Action.Data[0].Bool;
        world.GetBlockRef<FeaturePhysicsDynamicBlock>().bAllowSleep = allowSleep;

        return true;
    }

    return false;
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
            Value f = force / entityBody.BodyComponent->InvMass;
            entityBody.BodyComponent->LinearVelocity += dir.Normalized() * f * t;
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

    bodyComponent->LinearVelocity += force;
}

bool FeaturePhysics::GetDebugDrawContacts() const
{
    return PhysicsSystem && PhysicsSystem->bDebugDrawContacts;
}

void FeaturePhysics::SetDebugDrawContacts(const bool& value)
{
    if (PhysicsSystem)
    {
        PhysicsSystem->bDebugDrawContacts = value;
    }
}

bool FeaturePhysics::GetAllowSleep() const
{
    WorldSharedPtr worldPtr = Session->GetWorldManager()->GetPrimaryWorld();
    if (!worldPtr) return false;
    auto blockPtr = worldPtr->GetBlock<FeaturePhysicsDynamicBlock>();
    if (!blockPtr) return false;
    return blockPtr->bAllowSleep;
}

void FeaturePhysics::SetAllowSleep(const bool& value)
{
    Action action;
    action.Verb = "set_allow_sleep"_n;
    action.Data[0].Bool = value;
    Session->QueueAction(action);
}
