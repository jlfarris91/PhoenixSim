
#include "FeatureECS.h"

#include "Worlds.h"

// REMOVE ME!
#include "FeaturePhysics.h"

using namespace Phoenix;
using namespace Phoenix::ECS;

PHOENIXSIM_API const FName FeatureECS::StaticName = "ECS"_n;
PHOENIXSIM_API const FName FeatureECSDynamicBlock::StaticName = "ECS_Dynamic"_n;

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
    WorldBufferBlockArgs blockArgs;
    blockArgs.Name = FeatureECSDynamicBlock::StaticName;
    blockArgs.Size = sizeof(FeatureECSDynamicBlock);
    blockArgs.BlockType = EWorldBufferBlockType::Dynamic;

    FeatureDefinition.Name = StaticName;
    FeatureDefinition.Blocks.push_back(blockArgs);

    FeatureDefinition.Channels.emplace_back(WorldChannels::Update, FeatureInsertPosition::Default);
    FeatureDefinition.Channels.emplace_back(WorldChannels::HandleAction, FeatureInsertPosition::Default);
}

FeatureECS::FeatureECS(const FeatureECSCtorArgs& args)
    : FeatureECS()
{
    for (const TSharedPtr<ISystem>& system : args.Systems)
    {
        Systems.push_back(system);
    }
}

FName FeatureECS::GetName() const
{
    return StaticName;
}

FeatureDefinition FeatureECS::GetFeatureDefinition()
{
    return FeatureDefinition;
}

void FeatureECS::OnPreUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    IFeature::OnPreUpdate(world, args);

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnPreUpdate(world);
    }
}

void FeatureECS::OnUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    IFeature::OnUpdate(world, args);
    
    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnUpdate(world);
    }
}

void FeatureECS::OnPostUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    IFeature::OnPostUpdate(world, args);

    for (const TSharedPtr<ISystem>& system : Systems)
    {
        system->OnPostUpdate(world);
    }

    CompactWorldBuffer(world);
}

void FeatureECS::OnHandleAction(WorldRef world, const FeatureActionArgs& action)
{
    IFeature::OnHandleAction(world, action);

    if (action.Action.Verb == "spawn_entity"_n)
    {
        EntityId entityId = AcquireEntity(world, action.Action.Data[0].Name);
        Physics::BodyComponent* bodyComponent = AddComponent<Physics::BodyComponent>(world, entityId);
        bodyComponent->CollisionMask = 1;
        bodyComponent->Radius = 16;
        bodyComponent->Transform.Position.X = action.Action.Data[1].Distance;
        bodyComponent->Transform.Position.Y = action.Action.Data[2].Distance;
        bodyComponent->Transform.Rotation = action.Action.Data[3].Degrees;
        bodyComponent->InvMass = 1.0f;
        bodyComponent->Speed = (rand() % 1000) / 1000.0f * 1.0f;
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
    Entity& entity = block.Entities[index];
    return entity.Id == entityId ? &entity : nullptr;
}

const Entity* FeatureECS::GetEntityPtr(WorldConstRef world, EntityId entityId)
{
    const FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();
    uint32 index = GetEntityIndex(entityId);
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
    uint16 entityIdx = 1;
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
    entity.ComponentTail = INDEX_NONE;

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
    
    RemoveAllComponents(world, entityId);

    // Reset entity data
    Entity& entity = block.Entities[index];
    entity.Id = EntityId::Invalid;
    entity.Kind = FName::None;
    entity.ComponentTail = INDEX_NONE;

    return true;
}

int32 FeatureECS::GetIndexOfComponent(WorldConstRef world, EntityId entityId, FName componentType)
{
    int32 compIndex = INDEX_NONE;
    ForEachComponent(world, entityId, [&compIndex, componentType](const Component& comp, uint32 index)
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

Component* FeatureECS::GetComponentPtr(WorldRef world, EntityId entityId, FName componentType)
{
    int32 index = GetIndexOfComponent(world, entityId, componentType);
    return index != INDEX_NONE ? &world.GetBlockRef<FeatureECSDynamicBlock>().Components[index] : nullptr;
}

const Component* FeatureECS::GetComponentPtr(WorldConstRef world, EntityId entityId, FName componentType)
{
    int32 index = GetIndexOfComponent(world, entityId, componentType);
    return index != INDEX_NONE ? &world.GetBlockRef<FeatureECSDynamicBlock>().Components[index] : nullptr;
}

Component& FeatureECS::GetComponentRef(WorldRef world, EntityId entityId, FName componentType)
{
    int32 index = GetIndexOfComponent(world, entityId, componentType);
    PHX_ASSERT(index != INDEX_NONE);
    return world.GetBlockRef<FeatureECSDynamicBlock>().Components[index];
}

const Component& FeatureECS::GetComponentRef(WorldConstRef world, EntityId entityId, FName componentType)
{
    int32 index = GetIndexOfComponent(world, entityId, componentType);
    PHX_ASSERT(index != INDEX_NONE);
    return world.GetBlockRef<FeatureECSDynamicBlock>().Components[index];
}

bool FeatureECS::RemoveComponent(WorldRef world, EntityId entityId, FName componentType)
{
    int32 prevCompIndex = INDEX_NONE;
    int32 compIndex = INDEX_NONE;
    ForEachComponent(world, entityId, [&, componentType](const Component& comp, uint32 index)
    {
        compIndex = static_cast<int32>(index);
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
    Component& compToRemove = block.Components[compIndex];

    if (prevCompIndex != INDEX_NONE)
    {
        Component& prevComp = block.Components[prevCompIndex];
        PHX_ASSERT(prevComp.Owner == entityId);
        prevComp.Next = compToRemove.Next;
    }

    Entity& entity = GetEntityRef(world, entityId);
    if (entity.ComponentTail == compIndex)
    {
        entity.ComponentTail = prevCompIndex;
    }

    // Reset component data
    compToRemove.Owner = EntityId::Invalid;
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

    int32 compIndex = GetEntityIndex(entityId);

    uint32 numCompsRemoved = 0;
    while (compIndex != INDEX_NONE)
    {
        Component& comp = block.Components[compIndex];
        if (comp.Owner != entityId)
        {
            break;
        }

        compIndex = comp.Next;
        numCompsRemoved++;

        // Reset component data
        comp.Owner = EntityId::Invalid;
        comp.TypeName = FName::None;
        comp.Next = INDEX_NONE;
        memset(comp.Data, 0, sizeof(comp.Data));
    }

    entity->ComponentTail = INDEX_NONE;

    return numCompsRemoved;
}

void FeatureECS::RegisterSystem(const TSharedPtr<ISystem>& system)
{
    Systems.push_back(system);
}

Component* FeatureECS::AddComponent(WorldRef world, EntityId entityId, FName componentType)
{
    FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

    PHX_ASSERT(!block.Components.IsFull());

    Entity* entity = GetEntityPtr(world, entityId);
    if (!entity)
    {
        return nullptr;
    }

    int32 compIndex = entity->ComponentTail;

    // Find the next available component index
    if (compIndex == INDEX_NONE)
    {
        compIndex = GetEntityIndex(entityId);
    }
    else
    {
        compIndex = INDEX_NONE;
        for (int32 i = 1; i < ECS_MAX_COMPONENTS; ++i)
        {
            if (block.Components[i].Owner == EntityId::Invalid)
            {
                compIndex = i;
                break;
            }
        }

        if (compIndex == INDEX_NONE)
        {
            return nullptr;
        }
    }

    // Update the linked list
    if (entity->ComponentTail != INDEX_NONE)
    {
        block.Components[entity->ComponentTail].Next = compIndex;
    }

    entity->ComponentTail = compIndex;

    if (!block.Components.IsValidIndex(compIndex))
    {
        block.Components.SetNum(compIndex + 1);
    }

    Component& comp = block.Components[compIndex];
    comp.Owner = entityId;
    comp.Next = INDEX_NONE;
    comp.TypeName = componentType;

    // Clear out the component data
    std::memset(&comp.Data[0], 0, sizeof(Component::Data));
    
    return &comp;
}

void FeatureECS::CompactWorldBuffer(WorldRef world)
{
}
