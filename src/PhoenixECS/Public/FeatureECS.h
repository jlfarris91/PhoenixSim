
#pragma once

#include "DLLExport.h"
#include "Entity.h"
#include "Features.h"
#include "FixedTagList.h"
#include "ArchetypeManager.h"
#include "FeatureBlackboard.h"
#include "FixedBlackboard.h"
#include "FixedEntityList.h"
#include "Parallel.h"
#include "SystemJob.h"
#include "TransformComponent.h"

#ifndef PHX_ECS_MAX_ENTITIES
#define PHX_ECS_MAX_ENTITIES INT16_MAX
#endif

#ifndef PHX_ECS_MAX_TAGS
#define PHX_ECS_MAX_TAGS (INT16_MAX << 1)
#endif

namespace Phoenix
{
    namespace ECS
    {
        class ISystem;
        struct EntityQuery;

        struct PHOENIXECS_API FeatureECSDynamicBlock : BufferBlockBase
        {
            PHX_DECLARE_BLOCK_DYNAMIC(FeatureECSDynamicBlock)

            ArchetypeManager ArchetypeManager;
            FixedEntityList<PHX_ECS_MAX_ENTITIES> Entities;
            FixedTagList<PHX_ECS_MAX_TAGS> Tags;
            TFixedArray<EntityId, PHX_ECS_MAX_ENTITIES> Groups;
        };

        struct PHOENIXECS_API FeatureECSScratchBlock : BufferBlockBase
        {
            PHX_DECLARE_BLOCK_SCRATCH(FeatureECSScratchBlock)

            TFixedArray<EntityTransform, PHX_ECS_MAX_ENTITIES> SortedEntities;
            TAtomic<uint32> SortedEntityCount = 0;
        };

        struct PHOENIXECS_API FeatureECSCtorArgs
        {
            TArray<TSharedPtr<ISystem>> Systems;
        };

        class PHOENIXECS_API FeatureECS final : public IFeature
        {
        PHX_FEATURE_BEGIN(FeatureECS)
                FEATURE_WORLD_BLOCK(FeatureECSDynamicBlock)
                FEATURE_WORLD_BLOCK(FeatureECSScratchBlock)
                FEATURE_CHANNEL(FeatureChannels::WorldInitialize)
                FEATURE_CHANNEL(FeatureChannels::WorldShutdown)
                FEATURE_CHANNEL(FeatureChannels::PreWorldUpdate)
                FEATURE_CHANNEL(FeatureChannels::WorldUpdate)
                FEATURE_CHANNEL(FeatureChannels::PostWorldUpdate)
                FEATURE_CHANNEL(FeatureChannels::PreHandleWorldAction)
                FEATURE_CHANNEL(FeatureChannels::HandleWorldAction)
                FEATURE_CHANNEL(FeatureChannels::PostHandleWorldAction)
                FEATURE_CHANNEL(FeatureChannels::DebugRender)
                PHX_REGISTER_FIELD(bool, bDebugDrawMortonCodeBoundaries)
                PHX_REGISTER_FIELD(bool, bDebugDrawEntityZCodes)
            PHX_FEATURE_END()

        public:

            FeatureECS();
            FeatureECS(const FeatureECSCtorArgs& args);

            void OnPreUpdate(const FeatureUpdateArgs& args) override;
            void OnUpdate(const FeatureUpdateArgs& args) override;
            void OnPostUpdate(const FeatureUpdateArgs& args) override;

            bool OnPreHandleAction(const FeatureActionArgs& action) override;
            bool OnHandleAction(const FeatureActionArgs& action) override;
            bool OnPostHandleAction(const FeatureActionArgs& action) override;

            void OnWorldInitialize(WorldRef world) override;
            void OnWorldShutdown(WorldRef world) override;

            void OnPreWorldUpdate(WorldRef world, const FeatureUpdateArgs& args) override;
            void OnWorldUpdate(WorldRef world, const FeatureUpdateArgs& args) override;
            void OnPostWorldUpdate(WorldRef world, const FeatureUpdateArgs& args) override;

            bool OnPreHandleWorldAction(WorldRef world, const FeatureActionArgs& action) override;
            bool OnHandleWorldAction(WorldRef world, const FeatureActionArgs& action) override;
            bool OnPostHandleWorldAction(WorldRef world, const FeatureActionArgs& action) override;

            void OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer) override;

            //
            // System Management
            //

            // Registers a new ECS system.
            void RegisterSystem(const TSharedPtr<ISystem>& system);

            // Unregisters an existing ECS system. Returns true if the system was removed.
            bool UnregisterSystem(const TSharedPtr<ISystem>& system);

            //
            // Entity Management
            //
            
            static bool IsEntityValid(WorldConstRef world, EntityId entityId);
            
            static Entity* GetEntityPtr(WorldRef world, EntityId entityId);
            static const Entity* GetEntityPtr(WorldConstRef world, EntityId entityId);

            static Entity& GetEntityRef(WorldRef world, EntityId entityId);
            static const Entity& GetEntityRef(WorldConstRef world, EntityId entityId);

            static EntityId AcquireEntity(WorldRef world, const FName& kind);
            static bool ReleaseEntity(WorldRef world, EntityId entityId);

            static bool SetEntityKind(WorldRef world, EntityId entityId, const FName& kind);

            template <class ...TComponents>
            static void ForEachEntity(WorldRef world, const EntityQuery& query, const TEntityQueryFunc<TComponents...>& func)
            {
                FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
                if (!block)
                {
                    return;
                }

                return block->ArchetypeManager.ForEachEntity<TComponents...>(query, func);
            }

            template <class ...TComponents>
            static void ForEachEntity(WorldRef world, const EntityQuery& query, const TEntityQueryBufferFunc<TComponents...>& func)
            {
                FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
                if (!block)
                {
                    return;
                }

                return block->ArchetypeManager.ForEachEntity<TComponents...>(query, func);
            }

            //
            // Archetype Management
            //

            static bool RegisterArchetypeDefinition(WorldRef world, const ArchetypeDefinition& definition);

            template <class ...TComponents>
            static bool RegisterArchetypeDefinition(WorldRef world, const FName& id = FName::None)
            {
                return RegisterArchetypeDefinition(world, ArchetypeDefinition::Create<TComponents...>(id));
            }

            static bool UnregisterArchetypeDefinition(WorldRef world, const ArchetypeDefinition& definition);

            static bool HasArchetypeDefinition(WorldConstRef world, const FName& name);

            // Gets the pointer to a component on an entity if it exists.
            static IComponent* GetComponent(WorldRef world, EntityId entityId, const FName& componentType);

            // Gets the pointer to a component on an entity if it exists.
            static const IComponent* GetComponent(WorldConstRef world, EntityId entityId, const FName& componentType);

            // Gets a reference to a component on an entity if it exists.
            static IComponent& GetComponentRef(WorldRef world, EntityId entityId, const FName& componentType);

            // Gets a reference to a component on an entity if it exists.
            static const IComponent& GetComponentRef(WorldConstRef world, EntityId entityId, const FName& componentType);

            // Gets the pointer to a component on an entity if it exists.
            template <class T>
            static T* GetComponent(WorldRef world, EntityId entityId)
            {
                IComponent* comp = GetComponent(world, entityId, T::StaticTypeName);
                return static_cast<T*>(comp);
            }

            // Gets the pointer to a component on an entity if it exists.
            template <class T>
            static const T* GetComponent(WorldConstRef world, EntityId entityId)
            {
                const IComponent* comp = GetComponent(world, entityId, T::StaticTypeName);
                return static_cast<const T*>(comp);
            }

            // Gets a reference to a component on an entity if it exists.
            template <class T>
            static T& GetComponentRef(WorldRef world, EntityId entityId)
            {
                IComponent& comp = GetComponentRef(world, entityId, T::StaticName);
                return static_cast<T&>(comp);
            }

            // Gets a reference to a component on an entity if it exists.
            template <class T>
            static const T& GetComponentRef(WorldConstRef world, EntityId entityId)
            {
                const IComponent& comp = GetComponentRef(world, entityId, T::StaticName);
                return static_cast<const T&>(comp);
            }

            // Adds a new component to an entity.
            static IComponent* AddComponent(WorldRef world, EntityId entityId, const FName& componentType);

            // Adds a new component to an entity.
            template <class T>
            static T* AddComponent(WorldRef world, EntityId entityId, const T& defaultValue = {})
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

                return block->ArchetypeManager.AddComponent<T>(entity->Handle, defaultValue);
            }

            // Adds a new component to an entity.
            template <class T, class ...TArgs>
            static T* EmplaceComponent(WorldRef world, EntityId entityId, const TArgs&...args)
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

                return block->ArchetypeManager.EmplaceComponent<T, TArgs...>(entity->Handle, args...);
            }

            // Removes a component from an entity.
            static bool RemoveComponent(WorldRef world, EntityId entityId, const FName& componentType);

            // Removes all components from an entity, effectively releasing the associated archetype.
            static uint32 RemoveAllComponents(WorldRef world, EntityId entityId);

            // Enumerates all components on a given entity.
            template <class T>
            static void ForEachComponent(WorldRef world, EntityId entityId, const T& callback)
            {
                FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
                if (!block)
                {
                    return;
                }

                Entity* entity = GetEntityPtr(world, entityId);
                if (!entity)
                {
                    return;
                }

                return block->ArchetypeManager.ForEachComponent<T>(entity->Handle, callback);
            }

            // Enumerates all components on a given entity.
            template <class T>
            static void ForEachComponent(WorldConstRef world, EntityId entityId, const T& callback)
            {
                const FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
                if (!block)
                {
                    return;
                }

                const Entity* entity = GetEntityPtr(world, entityId);
                if (!entity)
                {
                    return;
                }

                return block->ArchetypeManager.ForEachComponent<T>(entity->Handle, callback);
            }

            //
            //  Tag Management
            //

            // Returns true if the entity has a given tag.
            static bool HasTag(WorldConstRef world, EntityId entityId, const FName& tagName);

            // Adds a tag to the entity. Returns true if the tag was added.
            static bool AddTag(WorldRef world, EntityId entityId, const FName& tagName);

            // Removes a tag from the entity. Returns true if the tag was removed.
            static bool RemoveTag(WorldRef world, EntityId entityId, const FName& tagName);

            // Removes all tags from the entity. Returns the number of tags that were removed.
            static uint32 RemoveAllTags(WorldRef world, EntityId entityId);

            // Enumerates each tag of a given entity.
            template <class TCallback>
            static void ForEachTag(WorldConstRef world, EntityId entityId, const TCallback& callback)
            {
                const FeatureECSDynamicBlock* block = world.GetBlock<FeatureECSDynamicBlock>();
                if (!block)
                {
                    return;
                }

                const Entity* entity = GetEntityPtr(world, entityId);
                if (!entity)
                {
                    return;
                }

                return block->Tags.ForEachTag(*entity, callback);
            }

            //
            // Blackboard helpers
            //

            static Blackboard::blackboard_key_t CreateBlackboardKey(
                const EntityId& id,
                const FName& key,
                Blackboard::blackboard_type_t type = Blackboard::UnknownType);

            static bool HasBlackboardValue(
                WorldConstRef world,
                const EntityId& id,
                const FName& key,
                Blackboard::blackboard_type_t type = Blackboard::UnknownType);

            template <class T>
            static bool HasBlackboardValue(WorldConstRef world, const EntityId& id, const FName& key)
            {
                return HasBlackboardValue(world, id, key, Blackboard::BlackboardValueType<T>::Type);
            }

            static bool SetBlackboardValue(
                WorldRef world,
                const EntityId& id,
                const FName& key,
                Blackboard::blackboard_value_t value,
                Blackboard::blackboard_type_t type = Blackboard::UnknownType);

            template <class T>
            static bool SetBlackboardValue(
                WorldRef world,
                const EntityId& id,
                const FName& key,
                const T& value)
            {
                Blackboard::WorldBlackboard& blackboard = Blackboard::FeatureBlackboard::GetBlackboard(world);
                Blackboard::blackboard_key_t fullKey = CreateBlackboardKey(id, key);
                return blackboard.SetValue<T>(fullKey, value);
            }

            static bool GetBlackboardValue(
                WorldConstRef world,
                const EntityId& id,
                const FName& key,
                Blackboard::blackboard_value_t& outValue,
                Blackboard::blackboard_type_t expectedType = Blackboard::IgnoreType);

            template <class T>
            static bool GetBlackboardValue(
                WorldConstRef world,
                const EntityId& id,
                const FName& key,
                T& outValue)
            {
                const Blackboard::WorldBlackboard& blackboard = Blackboard::FeatureBlackboard::GetBlackboard(world);
                Blackboard::blackboard_key_t fullKey = CreateBlackboardKey(id, key);
                return blackboard.GetValue<T>(fullKey, outValue);
            }

            template <class T = Blackboard::blackboard_value_t>
            static bool RemoveBlackboardValue(
                WorldRef world,
                const EntityId& id,
                const FName& key,
                bool checkType = true)
            {
                Blackboard::WorldBlackboard& blackboard = Blackboard::FeatureBlackboard::GetBlackboard(world);
                Blackboard::blackboard_key_t fullKey = CreateBlackboardKey(id, key);
                return blackboard.RemoveValue<T>(fullKey, checkType);
            }

            //
            // Jobs
            //

            template <class TJob>
            static void Schedule(WorldRef world, const TJob& job)
            {
                TSharedPtr<TaskQueue> taskQueue = TaskQueue::GetTaskQueue((uint32)world.GetName());

                FeatureECSDynamicBlock& dynamicBlock = world.GetBlockRef<FeatureECSDynamicBlock>();
                WorldPtr worldPtr = &world;

                uint32 startIndex = 0;
                dynamicBlock.ArchetypeManager.ForEachArchetypeList([&](auto& list)
                {
                    if (job.GetQuery().PassesFilter(list.GetDefinition()))
                    {
                        TJob jobInstance = job;
                        auto listPtr = &list;
                        auto wrapper = [=]() mutable
                        {
                            static_cast<IEntityJobBase*>(&jobInstance)->Execute(*worldPtr, *listPtr, startIndex);
                        };

                        taskQueue->Enqueue(std::move(wrapper));

                        startIndex += list.GetNumInstances();
                    }
                });
            }

            template <class TJob>
            static void ScheduleParallel(WorldRef world, const TJob& job)
            {
                PHX_PROFILE_ZONE_SCOPED;

                TSharedPtr<TaskQueue> taskQueue = TaskQueue::GetTaskQueue((uint32)world.GetName());

                FeatureECSDynamicBlock& dynamicBlock = world.GetBlockRef<FeatureECSDynamicBlock>();
                WorldPtr worldPtr = &world;

                uint32 numArchetypeLists = dynamicBlock.ArchetypeManager.GetNumArchetypeLists();
                std::vector<Task>& taskGroup = taskQueue->BeginGroup(numArchetypeLists);

                uint32 startIndex = 0;
                dynamicBlock.ArchetypeManager.ForEachArchetypeList([&](ArchetypeList& list)
                {
                    if (job.GetQuery().PassesFilter(list.GetDefinition()))
                    {
                        PHX_PROFILE_ZONE_SCOPED_N("PushTaskToTaskGroup");

                        TJob jobInstance = job;
                        auto listPtr = &list;
                        auto wrapper = [=]() mutable
                        {
                            static_cast<IEntityJobBase*>(&jobInstance)->Execute(*worldPtr, *listPtr, startIndex);
                        };

                        taskGroup.emplace_back(std::move(wrapper));

                        startIndex += list.GetNumInstances();
                    }
                });

                taskQueue->EndGroup();
            }

            static const Transform2D* GetLocalTransformPtr(WorldConstRef world, EntityId entityId);
            static const Transform2D* GetWorldTransformPtr(WorldConstRef world, EntityId entityId);

            static void QueryEntitiesInRange(WorldConstRef world, const Vec2& pos, Distance range, TArray<EntityTransform>& outEntities);

            bool bDebugDrawMortonCodeBoundaries = false;
            bool bDebugDrawEntityZCodes = false;

        private:

            static void SortEntitiesByZCode(WorldRef world);

            static void CompactWorldBuffer(WorldRef world);

            TArray<TSharedPtr<ISystem>> Systems;
            TSharedPtr<ThreadPool> JobThreadPool;
        };
    }
}
