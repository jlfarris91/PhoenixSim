
#include "FeatureECS.h"

#include "MortonCode.h"
#include "Profiling.h"
#include "System.h"

using namespace Phoenix;
using namespace Phoenix::ECS;

FeatureECS::FeatureECS()
{
}

FeatureECS::FeatureECS(const FeatureECSCtorArgs& args)
{
    for (const TSharedPtr<ISystem>& system : args.Systems)
    {
        Systems.push_back(system);
    }
}

void FeatureECS::OnPreUpdate(const FeatureUpdateArgs& args)
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

void FeatureECS::OnUpdate(const FeatureUpdateArgs& args)
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

void FeatureECS::OnPostUpdate(const FeatureUpdateArgs& args)
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

bool FeatureECS::OnPreHandleAction(const FeatureActionArgs& action)
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

bool FeatureECS::OnHandleAction(const FeatureActionArgs& action)
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

bool FeatureECS::OnPostHandleAction(const FeatureActionArgs& action)
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

void FeatureECS::OnWorldInitialize(WorldRef world)
{
    PHX_PROFILE_ZONE_SCOPED;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnWorldInitialize(world);
    }
}

void FeatureECS::OnWorldShutdown(WorldRef world)
{
    PHX_PROFILE_ZONE_SCOPED;

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnWorldShutdown(world);
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
        system->OnPreWorldUpdate(world, systemUpdateArgs);
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
        system->OnWorldUpdate(world, systemUpdateArgs);
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
        system->OnPostWorldUpdate(world, systemUpdateArgs);
    }

    CompactWorldBuffer(world);
}

bool FeatureECS::OnPreHandleWorldAction(WorldRef world, const FeatureActionArgs& action)
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

bool FeatureECS::OnHandleWorldAction(WorldRef world, const FeatureActionArgs& action)
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

bool FeatureECS::OnPostHandleWorldAction(WorldRef world, const FeatureActionArgs& action)
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

void FeatureECS::OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer)
{
    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnDebugRender(world, state, renderer);
    }
}

void FeatureECS::RegisterSystem(const TSharedPtr<ISystem>& system)
{
    Systems.push_back(system);
}

bool FeatureECS::UnregisterSystem(const TSharedPtr<ISystem>& system)
{
    auto iter = std::ranges::find(Systems, system);
    if (iter == Systems.end())
        return false;
    Systems.erase(iter);
    return true;
}

bool FeatureECS::IsEntityValid(WorldConstRef world, EntityId entityId)
{
    return GetEntityPtr(world, entityId) != nullptr;
}

Entity* FeatureECS::GetEntityPtr(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    return block.Entities.GetEntityPtr(entityId);
}

const Entity* FeatureECS::GetEntityPtr(WorldConstRef world, EntityId entityId)
{
    const FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    return block.Entities.GetEntityPtr(entityId);
}

Entity& FeatureECS::GetEntityRef(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    return block.Entities.GetEntityRef(entityId);
}

const Entity& FeatureECS::GetEntityRef(WorldConstRef world, EntityId entityId)
{
    const FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    return block.Entities.GetEntityRef(entityId);
}

EntityId FeatureECS::AcquireEntity(WorldRef world, const FName& kind)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

    EntityId entityId = block.Entities.Acquire(kind);
    if (entityId == EntityId::Invalid)
    {
        return EntityId::Invalid;
    }

    Entity& entity = block.Entities.GetEntityRef(entityId);

    // Automatically acquire an archetype if the kind matches one
    if (block.ArchetypeManager.IsArchetypeRegistered(kind))
    {
        entity.Handle = block.ArchetypeManager.Acquire(entityId, kind);
    }

    return entityId;
}

bool FeatureECS::ReleaseEntity(WorldRef world, EntityId entityId)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

    if (!block.Entities.IsValid(entityId))
    {
        return false;
    }

    Entity& entity = block.Entities.GetEntityRef(entityId);

    // If the entity has an archetype then release it now
    block.ArchetypeManager.Release(entity.Handle);

    // Remove all associated tags
    RemoveAllTags(world, entityId);

    block.Entities.Release(entityId);

    return true;
}

bool FeatureECS::SetEntityKind(WorldRef world, EntityId entityId, const FName& kind)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

    Entity* entity = block.Entities.GetEntityPtr(entityId);
    if (!entity)
    {
        return false;
    }

    if (entity->Kind == kind)
    {
        return true;
    }

    // If the entity has an archetype then release it now
    (void)block.ArchetypeManager.Release(entity->Handle);

    // Automatically acquire an archetype if the kind matches one
    if (block.ArchetypeManager.IsArchetypeRegistered(kind))
    {
        entity->Handle = block.ArchetypeManager.Acquire(entity->GetId(), kind);
    }

    return true;
}

EntityQueryBuilder<ArchetypeManager> FeatureECS::Entities(WorldRef world)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    return block.ArchetypeManager.Entities();
}

EntityQueryBuilder<ArchetypeManager> FeatureECS::Entities(WorldConstRef world)
{
    const FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    return block.ArchetypeManager.Entities();
}

bool FeatureECS::RegisterArchetypeDefinition(WorldRef world, const ArchetypeDefinition& definition)
{
    FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return false;
    }

    return block->ArchetypeManager.RegisterArchetypeDefinition(definition);
}

bool FeatureECS::UnregisterArchetypeDefinition(WorldRef world, const ArchetypeDefinition& definition)
{
    FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return false;
    }

    return block->ArchetypeManager.UnregisterArchetypeDefinition(definition);
}

bool FeatureECS::HasArchetypeDefinition(WorldConstRef world, const FName& name)
{
    const FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
    if (!block)
    {
        return false;
    }

    return block->ArchetypeManager.IsArchetypeRegistered(name);
}

IComponent* FeatureECS::GetComponent(
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

    return static_cast<IComponent*>(block->ArchetypeManager.GetComponent(entity->Handle, componentType));
}

const IComponent* FeatureECS::GetComponent(
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

    return static_cast<const IComponent*>(block->ArchetypeManager.GetComponent(entity->Handle, componentType));
}

IComponent& FeatureECS::GetComponentRef(
    WorldRef world,
    EntityId entityId,
    const FName& componentType)
{
    IComponent* component = GetComponent(world, entityId, componentType);
    PHX_ASSERT(component);
    return *component;
}

const IComponent& FeatureECS::GetComponentRef(
    WorldConstRef world,
    EntityId entityId,
    const FName& componentType)
{
    const IComponent* component = GetComponent(world, entityId, componentType);
    PHX_ASSERT(component);
    return *component;
}

IComponent* FeatureECS::AddComponent(
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

bool FeatureECS::RemoveComponent(WorldRef world, EntityId entityId, const FName& componentType)
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

uint32 FeatureECS::RemoveAllComponents(WorldRef world, EntityId entityId)
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
bool FeatureECS::HasTag(WorldConstRef world, EntityId entityId, const FName& tagName)
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

bool FeatureECS::AddTag(WorldRef world, EntityId entityId, const FName& tagName)
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

bool FeatureECS::RemoveTag(WorldRef world, EntityId entityId, const FName& tagName)
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

uint32 FeatureECS::RemoveAllTags(WorldRef world, EntityId entityId)
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
    
    FeatureECSScratchBlock& scratchBlock = world.GetBlockRef<FeatureECSScratchBlock>();
    
    // Calculate z-codes and sort entities
    scratchBlock.SortedEntities.Reset();
    Entities(world)
        .ForEachEntity(TFunction([&](const EntityComponentSpan<TransformComponent&>& span)
        {
            for (auto && [entityId, transformComp] : span)
            {
                transformComp.ZCode = ToMortonCode(transformComp.Transform.Position);
                scratchBlock.SortedEntities.EmplaceBack(entityId, &transformComp, transformComp.ZCode);
            }
        }));

    // Sort entities by their zcodes
    std::ranges::sort(
        scratchBlock.SortedEntities,
        [](const EntityTransform& a, const EntityTransform& b)
        {
            return a.ZCode < b.ZCode;
        });
}

void FeatureECS::CompactWorldBuffer(WorldRef world)
{
    FeatureECSDynamicBlock& dynamicBlock = world.GetBlockRef<FeatureECSDynamicBlock>();
    dynamicBlock.ArchetypeManager.Compact();
}
