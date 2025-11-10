
#include "FeatureECS2.h"

#include "MortonCode.h"
#include "Profiling.h"
#include "System.h"

Phoenix::ECS2::FeatureECS::FeatureECS()
{
}

Phoenix::ECS2::FeatureECS::FeatureECS(const FeatureECSCtorArgs& args)
{
    for (const TSharedPtr<ISystem>& system : args.Systems)
    {
        Systems.push_back(system);
    }
}

void Phoenix::ECS2::FeatureECS::OnPreUpdate(const FeatureUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemUpdateArgs systemUpdateArgs;
    systemUpdateArgs.SimTime = args.SimTime;
    systemUpdateArgs.DeltaTime = OneDivBy(Time(args.StepHz));

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnPreUpdate(systemUpdateArgs);
    }
}

void Phoenix::ECS2::FeatureECS::OnUpdate(const FeatureUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemUpdateArgs systemUpdateArgs;
    systemUpdateArgs.SimTime = args.SimTime;
    systemUpdateArgs.DeltaTime = OneDivBy(Time(args.StepHz));

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnUpdate(systemUpdateArgs);
    }
}

void Phoenix::ECS2::FeatureECS::OnPostUpdate(const FeatureUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemUpdateArgs systemUpdateArgs;
    systemUpdateArgs.SimTime = args.SimTime;
    systemUpdateArgs.DeltaTime = OneDivBy(Time(args.StepHz));

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnPostUpdate(systemUpdateArgs);
    }
}

bool Phoenix::ECS2::FeatureECS::OnPreHandleAction(const FeatureActionArgs& action)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemActionArgs systemActionArgs;
    systemActionArgs.SimTime = action.SimTime;
    systemActionArgs.Action = action.Action;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        if (system->OnPreHandleAction(systemActionArgs))
        {
            return true;
        }
    }
    
    return false;
}

bool Phoenix::ECS2::FeatureECS::OnHandleAction(const FeatureActionArgs& action)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemActionArgs systemActionArgs;
    systemActionArgs.SimTime = action.SimTime;
    systemActionArgs.Action = action.Action;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        if (system->OnHandleAction(systemActionArgs))
        {
            return true;
        }
    }
    
    return false;
}

bool Phoenix::ECS2::FeatureECS::OnPostHandleAction(const FeatureActionArgs& action)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemActionArgs systemActionArgs;
    systemActionArgs.SimTime = action.SimTime;
    systemActionArgs.Action = action.Action;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        if (system->OnPostHandleAction(systemActionArgs))
        {
            return true;
        }
    }
    
    return false;
}

void Phoenix::ECS2::FeatureECS::OnWorldInitialize(WorldRef world)
{
    PHX_PROFILE_ZONE_SCOPED;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnWorldInitialize(world);
    }
}

void Phoenix::ECS2::FeatureECS::OnWorldShutdown(WorldRef world)
{
    PHX_PROFILE_ZONE_SCOPED;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnWorldShutdown(world);
    }
}

void Phoenix::ECS2::FeatureECS::OnPreWorldUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    SortEntitiesByZCode(world);

    SystemUpdateArgs systemUpdateArgs;
    systemUpdateArgs.SimTime = args.SimTime;
    systemUpdateArgs.DeltaTime = OneDivBy(Time(args.StepHz));

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnPreWorldUpdate(world, systemUpdateArgs);
    }
}

void Phoenix::ECS2::FeatureECS::OnWorldUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemUpdateArgs systemUpdateArgs;
    systemUpdateArgs.SimTime = args.SimTime;
    systemUpdateArgs.DeltaTime = OneDivBy(Time(args.StepHz));
    
    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnWorldUpdate(world, systemUpdateArgs);
    }
}

void Phoenix::ECS2::FeatureECS::OnPostWorldUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemUpdateArgs systemUpdateArgs;
    systemUpdateArgs.SimTime = args.SimTime;
    systemUpdateArgs.DeltaTime = OneDivBy(Time(args.StepHz));
    
    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnPostWorldUpdate(world, systemUpdateArgs);
    }

    CompactWorldBuffer(world);
}

bool Phoenix::ECS2::FeatureECS::OnPreHandleWorldAction(WorldRef world, const FeatureActionArgs& action)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemActionArgs systemActionArgs;
    systemActionArgs.SimTime = action.SimTime;
    systemActionArgs.Action = action.Action;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        if (system->OnPreHandleWorldAction(world, systemActionArgs))
        {
            return true;
        }
    }
    
    return false;
}

bool Phoenix::ECS2::FeatureECS::OnHandleWorldAction(WorldRef world, const FeatureActionArgs& action)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemActionArgs systemActionArgs;
    systemActionArgs.SimTime = action.SimTime;
    systemActionArgs.Action = action.Action;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        if (system->OnHandleWorldAction(world, systemActionArgs))
        {
            return true;
        }
    }
    
    return false;
}

bool Phoenix::ECS2::FeatureECS::OnPostHandleWorldAction(WorldRef world, const FeatureActionArgs& action)
{
    PHX_PROFILE_ZONE_SCOPED;

    SystemActionArgs systemActionArgs;
    systemActionArgs.SimTime = action.SimTime;
    systemActionArgs.Action = action.Action;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        if (system->OnPostHandleWorldAction(world, systemActionArgs))
        {
            return true;
        }
    }
    
    return false;
}

void Phoenix::ECS2::FeatureECS::OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer)
{
    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnDebugRender(world, state, renderer);
    }
}

void Phoenix::ECS2::FeatureECS::RegisterSystem(const TSharedPtr<ISystem>& system)
{
    Systems.push_back(system);
}

bool Phoenix::ECS2::FeatureECS::UnregisterSystem(const TSharedPtr<ISystem>& system)
{
    auto iter = std::ranges::find(Systems, system);
    if (iter == Systems.end())
        return false;
    Systems.erase(iter);
    return true;
}

bool Phoenix::ECS2::FeatureECS::IsEntityValid(WorldConstRef world, EntityId entityId)
{
    return GetEntityPtr(world, entityId) != nullptr;
}

Phoenix::int32 Phoenix::ECS2::FeatureECS::GetEntityIndex(EntityId entityId)
{
    return entityId % decltype(FeatureECSDynamicBlock::Entities)::Capacity;
}

Phoenix::ECS2::Entity* Phoenix::ECS2::FeatureECS::GetEntityPtr(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    uint32 index = GetEntityIndex(entityId);
    if (!block.Entities.IsValidIndex(index))
        return nullptr;
    Entity& entity = block.Entities[index];
    return entity.GetId() == entityId ? &entity : nullptr;
}

const Phoenix::ECS2::Entity* Phoenix::ECS2::FeatureECS::GetEntityPtr(WorldConstRef world, EntityId entityId)
{
    const FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    uint32 index = GetEntityIndex(entityId);
    if (!block.Entities.IsValidIndex(index))
        return nullptr;
    const Entity& entity = block.Entities[index];
    return entity.GetId() == entityId ? &entity : nullptr;
}

Phoenix::ECS2::Entity& Phoenix::ECS2::FeatureECS::GetEntityRef(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    uint32 index = GetEntityIndex(entityId);
    Entity& entity = block.Entities[index];
    return entity.GetId() == entityId ? entity : block.Entities[0];
}

const Phoenix::ECS2::Entity& Phoenix::ECS2::FeatureECS::GetEntityRef(WorldConstRef world, EntityId entityId)
{
    const FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    uint32 index = GetEntityIndex(entityId);
    const Entity& entity = block.Entities[index];
    return entity.GetId() == entityId ? entity : block.Entities[0];
}

Phoenix::ECS2::EntityId Phoenix::ECS2::FeatureECS::AcquireEntity(WorldRef world, const FName& kind)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

    // Find the first invalid entity index
    uint32 entityIdx = 1;
    for (; entityIdx < PHX_ECS_MAX_ENTITIES; ++entityIdx)
    {
        if (block.Entities[entityIdx].GetId() == EntityId::Invalid)
        {
            break;
        }
    }

    if (entityIdx == PHX_ECS_MAX_ENTITIES)
    {
        return EntityId::Invalid;
    }

    if (!block.Entities.IsValidIndex(entityIdx))
    {
        block.Entities.SetNum(entityIdx + 1);
    }

    Entity& entity = block.Entities[entityIdx];
    entity.Kind = kind;
    entity.Handle = ArchetypeHandle();
    entity.TagHead = INDEX_NONE;

    // Automatically acquire an archetype if the kind matches one
    if (block.ArchetypeManager.HasArchetypeDefinition(kind))
    {
        entity.Handle = block.ArchetypeManager.Acquire(entityIdx, kind);
    }

    return entityIdx;
}

bool Phoenix::ECS2::FeatureECS::ReleaseEntity(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    int32 index = GetEntityIndex(entityId);
    if (index == INDEX_NONE)
    {
        return false;
    }

    Entity& entity = block.Entities[index];
    entity.Kind = FName::None;

    // If the entity has an archetype then release it now
    block.ArchetypeManager.Release(entity.Handle);
    entity.Handle = ArchetypeHandle();

    // Remove any tags
    RemoveAllTags(world, entityId);

    return true;
}

bool Phoenix::ECS2::FeatureECS::SetEntityKind(WorldRef world, EntityId entityId, const FName& kind)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    int32 index = GetEntityIndex(entityId);
    if (index == INDEX_NONE)
    {
        return false;
    }

    Entity& entity = block.Entities[index];
    if (entity.Kind == kind)
    {
        return true;
    }

    // If the entity has an archetype then release it now
    block.ArchetypeManager.Release(entity.Handle);

    // Automatically acquire an archetype if the kind matches one
    if (block.ArchetypeManager.HasArchetypeDefinition(kind))
    {
        entity.Handle = block.ArchetypeManager.Acquire(entity.GetId(), kind);
    }

    return true;
}

Phoenix::ECS2::IComponent* Phoenix::ECS2::FeatureECS::GetComponentPtr(
    WorldRef world,
    EntityId entityId,
    const FName& componentType)
{
    FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return nullptr;
    }

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return nullptr;
    }

    return static_cast<IComponent*>(block->ArchetypeManager.GetComponentPtr(entity->Handle, componentType));
}

const Phoenix::ECS2::IComponent* Phoenix::ECS2::FeatureECS::GetComponentPtr(
    WorldConstRef world,
    EntityId entityId,
    const FName& componentType)
{
    const FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return nullptr;
    }

    const Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return nullptr;
    }

    return static_cast<const IComponent*>(block->ArchetypeManager.GetComponentPtr(entity->Handle, componentType));
}

Phoenix::ECS2::IComponent& Phoenix::ECS2::FeatureECS::GetComponentRef(
    WorldRef world,
    EntityId entityId,
    const FName& componentType)
{
    IComponent* component = GetComponentPtr(world, entityId, componentType);
    PHX_ASSERT(component);
    return *component;
}

const Phoenix::ECS2::IComponent& Phoenix::ECS2::FeatureECS::GetComponentRef(
    WorldConstRef world,
    EntityId entityId,
    const FName& componentType)
{
    const IComponent* component = GetComponentPtr(world, entityId, componentType);
    PHX_ASSERT(component);
    return *component;
}

Phoenix::ECS2::IComponent* Phoenix::ECS2::FeatureECS::AddComponent(
    WorldRef world,
    EntityId entityId,
    const FName& componentType)
{
    FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return nullptr;
    }

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return nullptr;
    }

    return static_cast<IComponent*>(block->ArchetypeManager.AddComponent(entity->Handle, componentType));
}

bool Phoenix::ECS2::FeatureECS::RemoveComponent(WorldRef world, EntityId entityId, const FName& componentType)
{
    FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return false;
    }

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return false;
    }

    return block->ArchetypeManager.RemoveComponent(entity->Handle, componentType);
}

Phoenix::uint32 Phoenix::ECS2::FeatureECS::RemoveAllComponents(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return false;
    }

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return false;
    }

    return block->ArchetypeManager.RemoveAllComponents(entity->Handle);
}
bool Phoenix::ECS2::FeatureECS::HasTag(WorldConstRef world, EntityId entityId, const FName& tagName)
{
    const FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return false;
    }

    const Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return false;
    }

    return block->Tags.HasTag(*entity, tagName);
}

bool Phoenix::ECS2::FeatureECS::AddTag(WorldRef world, EntityId entityId, const FName& tagName)
{
    FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return false;
    }

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return false;
    }

    return block->Tags.AddTag(*entity, tagName);
}

bool Phoenix::ECS2::FeatureECS::RemoveTag(WorldRef world, EntityId entityId, const FName& tagName)
{
    FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return false;
    }

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return false;
    }

    return block->Tags.RemoveTag(*entity, tagName);
}

Phoenix::uint32 Phoenix::ECS2::FeatureECS::RemoveAllTags(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return 0;
    }

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return 0;
    }

    return block->Tags.RemoveAllTags(*entity);
}

void Phoenix::ECS2::FeatureECS::QueryEntitiesInRange(
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

void Phoenix::ECS2::FeatureECS::SortEntitiesByZCode(WorldRef world)
{
    // PHX_PROFILE_ZONE_SCOPED;
    //
    // FeatureECSScratchBlock* scratchBlock = world.GetBlock<FeatureECSScratchBlock>();
    //
    // // Gather all entities with transform components
    // {
    //     scratchBlock->EntityTransforms.Refresh(world);
    // }
    //
    // // Calculate z-codes and sort entities
    // scratchBlock->SortedEntities.Reset();
    // for (auto && [entity, transformComp] : scratchBlock->EntityTransforms)
    // {
    //     transformComp->ZCode = ToMortonCode(transformComp->Transform.Position);
    //     scratchBlock->SortedEntities.EmplaceBack(entity->Id, transformComp, transformComp->ZCode);
    // }
}

void Phoenix::ECS2::FeatureECS::CompactWorldBuffer(WorldRef world)
{
}
