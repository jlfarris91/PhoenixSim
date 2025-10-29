
#include "FeatureECS.h"

#include <cstring>  // For memset
#include <cstdio>   // For snprintf

#include "Worlds.h"

// REMOVE ME!
#include "Color.h"
#include "Debug.h"
#include "FeaturePhysics.h"
#include "Flags.h"
#include "MortonCode.h"
#include "Profiling.h"

using namespace Phoenix;
using namespace Phoenix::ECS;

PHOENIXSIM_API const EntityId EntityId::Invalid = 0;

EntityId::operator entityid_t() const
{
    return Id;
}

EntityId& EntityId::operator=(const entityid_t& id)
{
    Id = id;
    return *this;
}

FeatureECS::FeatureECS()
{
}

FeatureECS::FeatureECS(const FeatureECSCtorArgs& args)
    : FeatureECS()
{
    for (const TSharedPtr<ISystem>& system : args.Systems)
    {
        Systems.push_back(system);
    }
}

void FeatureECS::OnPreWorldUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    SortEntitiesByZCode(world);

    SystemUpdateArgs systemUpdateArgs;
    systemUpdateArgs.SimTime = args.SimTime;
    systemUpdateArgs.DeltaTime = OneDivBy(Time(args.StepHz));

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnPreUpdate(world, systemUpdateArgs);
    }
}

void FeatureECS::OnWorldUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemUpdateArgs systemUpdateArgs;
    systemUpdateArgs.SimTime = args.SimTime;
    systemUpdateArgs.DeltaTime = OneDivBy(Time(args.StepHz));
    
    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnUpdate(world, systemUpdateArgs);
    }
}

void FeatureECS::OnPostWorldUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemUpdateArgs systemUpdateArgs;
    systemUpdateArgs.SimTime = args.SimTime;
    systemUpdateArgs.DeltaTime = OneDivBy(Time(args.StepHz));

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnPostUpdate(world, systemUpdateArgs);
    }

    CompactWorldBuffer(world);
}

bool FeatureECS::OnHandleWorldAction(WorldRef world, const FeatureActionArgs& action)
{
    IFeature::OnHandleWorldAction(world, action);

    // TODO (jfarris): move this to script
    if (action.Action.Verb == "spawn_entity"_n)
    {   
        for (uint32 i = 0; i < action.Action.Data[4].UInt32; ++i)
        {
            EntityId entityId = AcquireEntity(world, action.Action.Data[0].Name);
            if (entityId == EntityId::Invalid)
                break;

            TransformComponent* transformComp = AddComponent<TransformComponent>(world, entityId);
            transformComp->Transform.Position.X = action.Action.Data[1].Distance;
            transformComp->Transform.Position.Y = action.Action.Data[2].Distance;
            transformComp->Transform.Rotation = action.Action.Data[3].Degrees;

            Physics::BodyComponent* bodyComp = AddComponent<Physics::BodyComponent>(world, entityId);
            bodyComp->CollisionMask = 1;
            bodyComp->Radius = 0.6; // Lancer :)
            bodyComp->InvMass = OneDivBy<Value>(1.0f);
            bodyComp->LinearDamping = 5.f;
            SetFlagRef(bodyComp->Flags, Physics::EBodyFlags::Awake, true);
        }

        return true;
    }

    SystemActionArgs systemActionArgs;
    systemActionArgs.SimTime = action.SimTime;
    systemActionArgs.Action = action.Action;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        if (system->OnHandleAction(world, systemActionArgs))
        {
            return true;
        }
    }

    return false;
}

void FeatureECS::OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer)
{
    IFeature::OnDebugRender(world, state, renderer);

    if (bDebugDrawMortonCodeBoundaries)
    {
        TVec2<Value> viewportSize(Distance::Max, Distance::Max);

        Distance step = (1 << MortonCodeGridBits);
        uint32 steps = (uint32)(Distance::Max / step);

        for (uint32 i = 0; i < steps; ++i)
        {
            Distance x = (int)i * step;
            renderer.DrawLine(Vec2(x, 0), Vec2(x, viewportSize.Y), Color(30, 30, 30));
            Distance y = (int)i * step;
            renderer.DrawLine(Vec2(0, y), Vec2(viewportSize.X, y), Color(30, 30, 30));
        }
    }

    if (bDebugDrawEntityZCodes)
    {
        const FeatureECSScratchBlock& scratchBlock = world.GetBlockRef<FeatureECSScratchBlock>();
        for (const auto& entityTransform : scratchBlock.SortedEntities)
        {
            Vec2 pt = entityTransform.TransformComponent->Transform.Position + Vec2::YAxis;

            uint8 quad = GetMortonCodeQuad(entityTransform.ZCode);
            uint64 zcode = GetMortonCodeValue(entityTransform.ZCode);

            char zcodeStr[256] = { '\0' };
#ifdef _WIN32
            sprintf_s(zcodeStr, _countof(zcodeStr), "%u:%llu", quad, zcode);
#else
            snprintf(zcodeStr, sizeof(zcodeStr), "%u:%llu", quad, zcode);
#endif
            renderer.DrawDebugText(pt, zcodeStr, _countof(zcodeStr), Color::White);
        }
    }

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnDebugRender(world, state, renderer);
    }
}

bool FeatureECS::IsEntityValid(WorldConstRef world, EntityId entityId)
{
    return GetEntityPtr(world, entityId) != nullptr;
}

int32 FeatureECS::GetEntityIndex(EntityId entityId)
{
    return entityId % decltype(FeatureECSDynamicBlock::Entities)::Capacity;
}

Entity* FeatureECS::GetEntityPtr(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    uint32 index = GetEntityIndex(entityId);
    if (!block.Entities.IsValidIndex(index))
        return nullptr;
    Entity& entity = block.Entities[index];
    return entity.Id == entityId ? &entity : nullptr;
}

const Entity* FeatureECS::GetEntityPtr(WorldConstRef world, EntityId entityId)
{
    const FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    uint32 index = GetEntityIndex(entityId);
    if (!block.Entities.IsValidIndex(index))
        return nullptr;
    const Entity& entity = block.Entities[index];
    return entity.Id == entityId ? &entity : nullptr;
}

Entity& FeatureECS::GetEntityRef(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    uint32 index = GetEntityIndex(entityId);
    Entity& entity = block.Entities[index];
    return entity.Id == entityId ? entity : block.Entities[0];
}

const Entity& FeatureECS::GetEntityRef(WorldConstRef world, EntityId entityId)
{
    const FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    uint32 index = GetEntityIndex(entityId);
    const Entity& entity = block.Entities[index];
    return entity.Id == entityId ? entity : block.Entities[0];
}

EntityId FeatureECS::AcquireEntity(WorldRef world, FName kind)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

    // Find the first invalid entity index
    uint32 entityIdx = 1;
    for (; entityIdx < ECS_MAX_ENTITIES; ++entityIdx)
    {
        if (block.Entities[entityIdx].Id == EntityId::Invalid)
        {
            break;
        }
    }

    if (entityIdx == ECS_MAX_ENTITIES)
    {
        return EntityId::Invalid;
    }

    if (!block.Entities.IsValidIndex(entityIdx))
    {
        block.Entities.SetNum(entityIdx + 1);
    }

    Entity& entity = block.Entities[entityIdx];
    entity.Id = entityIdx;
    entity.Kind = kind;
    entity.ComponentHead = INDEX_NONE;

    return entityIdx;
}

bool FeatureECS::ReleaseEntity(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    int32 index = GetEntityIndex(entityId);
    if (index == INDEX_NONE)
    {
        return false;
    }
    
    RemoveAllTags(world, entityId);
    RemoveAllComponents(world, entityId);

    // Reset entity data
    Entity& entity = block.Entities[index];
    entity.Id = EntityId::Invalid;
    entity.Kind = FName::None;
    entity.ComponentHead = INDEX_NONE;

    return true;
}

bool FeatureECS::HasTag(WorldConstRef world, EntityId entityId, FName tagName)
{
    bool foundTag = false;
    ForEachTag(world, entityId, [&](const EntityTag& tag, int32)
    {
        if (tag.TagName == tagName)
        {
            foundTag = true;
            return false;
        }
        return true;
    });
    return foundTag;
}

bool FeatureECS::AddTag(WorldRef world, EntityId entityId, FName tagName)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

    PHX_ASSERT(!block.Tags.IsFull());

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return false;
    }

    int32 newTagIndex = INDEX_NONE;

    // Find the next available tag index
    {
        for (int32 i = 1; i < ECS_MAX_TAGS; ++i)
        {
            if (block.Tags[i].TagName == FName::None)
            {
                newTagIndex = i;
                break;
            }
        }

        if (newTagIndex == INDEX_NONE)
        {
            return false;
        }
    }

    if (entity->TagHead == INDEX_NONE)
    {
        entity->TagHead = newTagIndex;
    }
    else
    {
        // Find the tail tag and update it's next
        int32 tagIter = entity->TagHead;
        while (block.Tags[tagIter].Next != INDEX_NONE)
        {
            // Entity already has the tag, don't add duplicates
            if (block.Tags[tagIter].TagName == tagName)
            {
                return false;
            }

            tagIter = block.Tags[tagIter].Next;
        }
        block.Tags[tagIter].Next = newTagIndex;
    }

    // Make room for the new tag if necessary
    if (!block.Tags.IsValidIndex(newTagIndex))
    {
        block.Tags.SetNum(newTagIndex + 1);
    }

    EntityTag& tag = block.Tags[newTagIndex];
    tag.Next = INDEX_NONE;
    tag.TagName = tagName;

    return true;
}

bool FeatureECS::RemoveTag(WorldRef world, EntityId entityId, FName tagName)
{
    int32 prevTagIndex = INDEX_NONE;
    int32 tagIndex = INDEX_NONE;
    ForEachTag(world, entityId, [&, tagName](const EntityTag& tag, uint32 index)
    {
        tagIndex = index;
        if (tag.TagName == tagName)
        {
            return false;
        }
        prevTagIndex = tagIndex;
        return true;
    });

    if (tagIndex == INDEX_NONE)
    {
        return false;
    }

    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    EntityTag& tagToRemove = block.Tags[tagIndex];

    if (prevTagIndex != INDEX_NONE)
    {
        EntityTag& prevTag = block.Tags[prevTagIndex];
        prevTag.Next = tagToRemove.Next;
    }

    Entity& entity = GetEntityRef(world, entityId);
    if (entity.TagHead == tagIndex)
    {
        entity.TagHead = prevTagIndex;
    }

    // Reset tag data
    tagToRemove.TagName = FName::None;
    tagToRemove.Next = INDEX_NONE;

    return true;
}

uint32 FeatureECS::RemoveAllTags(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return false;
    }

    int32 tagIndex = entity->TagHead;

    uint32 numTagsRemoved = 0;
    while (tagIndex != INDEX_NONE)
    {
        EntityTag& tag = block.Tags[tagIndex];

        tagIndex = tag.Next;
        numTagsRemoved++;

        // Reset tag data
        tag.TagName = FName::None;
        tag.Next = INDEX_NONE;
    }

    entity->TagHead = INDEX_NONE;

    return numTagsRemoved;
}

int32 FeatureECS::GetIndexOfComponent(WorldConstRef world, EntityId entityId, FName componentType)
{
    int32 compIndex = INDEX_NONE;
    ForEachComponent(world, entityId, [&compIndex, componentType](const EntityComponent& comp, uint32 index)
    {
        if (comp.TypeName == componentType)
        {
            compIndex = static_cast<int32>(index);
            return false;
        }
        return true;
    });

    return compIndex;
}

EntityComponent* FeatureECS::GetComponentPtr(WorldRef world, EntityId entityId, FName componentType)
{
    int32 index = GetIndexOfComponent(world, entityId, componentType);
    return index != INDEX_NONE ? &world.GetBlockRef<FeatureECSDynamicBlock>().Components[index] : nullptr;
}

const EntityComponent* FeatureECS::GetComponentPtr(WorldConstRef world, EntityId entityId, FName componentType)
{
    int32 index = GetIndexOfComponent(world, entityId, componentType);
    return index != INDEX_NONE ? &world.GetBlockRef<FeatureECSDynamicBlock>().Components[index] : nullptr;
}

EntityComponent& FeatureECS::GetComponentRef(WorldRef world, EntityId entityId, FName componentType)
{
    int32 index = GetIndexOfComponent(world, entityId, componentType);
    PHX_ASSERT(index != INDEX_NONE);
    return world.GetBlockRef<FeatureECSDynamicBlock>().Components[index];
}

const EntityComponent& FeatureECS::GetComponentRef(WorldConstRef world, EntityId entityId, FName componentType)
{
    int32 index = GetIndexOfComponent(world, entityId, componentType);
    PHX_ASSERT(index != INDEX_NONE);
    return world.GetBlockRef<FeatureECSDynamicBlock>().Components[index];
}

bool FeatureECS::RemoveComponent(WorldRef world, EntityId entityId, FName componentType)
{
    int32 prevCompIndex = INDEX_NONE;
    int32 compIndex = INDEX_NONE;
    ForEachComponent(world, entityId, [&, componentType](const EntityComponent& comp, uint32 index)
    {
        compIndex = index;
        if (comp.TypeName == componentType)
        {
            return false;
        }
        prevCompIndex = compIndex;
        return true;
    });

    if (compIndex == INDEX_NONE)
    {
        return false;
    }

    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    EntityComponent& compToRemove = block.Components[compIndex];

    if (prevCompIndex != INDEX_NONE)
    {
        EntityComponent& prevComp = block.Components[prevCompIndex];
        prevComp.Next = compToRemove.Next;
    }

    Entity& entity = GetEntityRef(world, entityId);
    if (entity.ComponentHead == compIndex)
    {
        entity.ComponentHead = prevCompIndex;
    }

    // Reset component data
    compToRemove.TypeName = FName::None;
    compToRemove.Next = INDEX_NONE;
    memset(compToRemove.Data, 0, sizeof(compToRemove.Data));

    return true;
}

uint32 FeatureECS::RemoveAllComponents(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return false;
    }

    int32 compIndex = entity->ComponentHead;

    uint32 numCompsRemoved = 0;
    while (compIndex != INDEX_NONE)
    {
        EntityComponent& comp = block.Components[compIndex];

        compIndex = comp.Next;
        numCompsRemoved++;

        // Reset component data
        comp.TypeName = FName::None;
        comp.Next = INDEX_NONE;
        memset(comp.Data, 0, sizeof(comp.Data));
    }

    entity->ComponentHead = INDEX_NONE;

    return numCompsRemoved;
}

void FeatureECS::RegisterSystem(const TSharedPtr<ISystem>& system)
{
    Systems.push_back(system);
}

EntityComponent* FeatureECS::AddComponent(WorldRef world, EntityId entityId, FName componentType)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

    PHX_ASSERT(!block.Components.IsFull());

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return nullptr;
    }

    int32 newCompIndex = INDEX_NONE;

    // Find the next available component index
    {
        for (int32 i = 1; i < ECS_MAX_COMPONENTS; ++i)
        {
            if (block.Components[i].TypeName == FName::None)
            {
                newCompIndex = i;
                break;
            }
        }

        if (newCompIndex == INDEX_NONE)
        {
            return nullptr;
        }
    }

    if (entity->ComponentHead == INDEX_NONE)
    {
        entity->ComponentHead = newCompIndex;
    }
    else
    {
        // Find the tail component and update it's next
        int32 compIter = entity->ComponentHead;
        while (block.Components[compIter].Next != INDEX_NONE)
        {
            compIter = block.Components[compIter].Next;
        }
        block.Components[compIter].Next = newCompIndex;
    }

    // Make room for the new component if necessary
    if (!block.Components.IsValidIndex(newCompIndex))
    {
        block.Components.SetNum(newCompIndex + 1);
    }

    EntityComponent& comp = block.Components[newCompIndex];
    comp.Next = INDEX_NONE;
    comp.TypeName = componentType;

    // Clear out the component data
    std::memset(&comp.Data[0], 0, sizeof(EntityComponent::Data));
    
    return &comp;
}

void FeatureECS::GetWorldTransform(WorldConstRef world, EntityId entityId, Transform2D& outTransform)
{
    
}

void FeatureECS::QueryEntitiesInRange(
    WorldConstRef world,
    const Vec2& pos,
    Distance range,
    TArray<EntityTransform>& outEntities)
{
    PHX_PROFILE_ZONE_SCOPED;

    const FeatureECSScratchBlock& scratchBlock = world.GetBlockRef<FeatureECSScratchBlock>();

    // Query for overlapping morton ranges
    TMortonCodeRangeArray ranges;
    MortonCodeAABB aabb = ToMortonCodeAABB(pos, range);
    MortonCodeQuery(aabb, ranges);

    TArray<EntityTransform*> overlappingEntities;
    ForEachInMortonCodeRanges<EntityTransform, &EntityTransform::ZCode>(
        scratchBlock.SortedEntities,
        ranges,
        [&](const EntityTransform& entityBody)
        {
            outEntities.push_back(entityBody);
        });
    
}

void FeatureECS::SortEntitiesByZCode(WorldRef world)
{
    PHX_PROFILE_ZONE_SCOPED;

    FeatureECSScratchBlock* scratchBlock = world.GetBlock<FeatureECSScratchBlock>();
    
    // Gather all entities with transform components
    {
        scratchBlock->EntityTransforms.Refresh(world);
    }

    // Calculate z-codes and sort entities
    scratchBlock->SortedEntities.Reset();
    for (auto && [entity, transformComp] : scratchBlock->EntityTransforms)
    {
        transformComp->ZCode = ToMortonCode(transformComp->Transform.Position);
        scratchBlock->SortedEntities.EmplaceBack(entity->Id, transformComp, transformComp->ZCode);
    }
}

void FeatureECS::CompactWorldBuffer(WorldRef world)
{
}
