
#pragma once

#include "ArchetypeDefinition.h"
#include "ArchetypeList.h"
#include "EntityQuery.h"
#include "Platform.h"
#include "Containers/FixedMap.h"
#include "Name.h"
#include "Session.h"
#include "Containers/FixedChunkAllocator.h"
#include "Utils.h"
#include "Containers/FixedArena.h"

namespace Phoenix
{
    namespace ECS2
    {
        template <
            class TArchetypeDefinition = TArchetypeDefinition<8>,
            uint16 MaxNumArchetypes = 32,
            uint16 MaxNumArchetypeLists = 1024,
            uint32 ArchetypeListSize = 16000
        >
        class TArchetypeManager
        {
        public:

            using TArchetypeList = TArchetypeList<TArchetypeDefinition, ArchetypeListSize>;
            using TChunkAllocator = TFixedChunkAllocator<MaxNumArchetypeLists, sizeof(TArchetypeList)>;
            using TChunkHandle = typename TChunkAllocator::Handle;
            using TEntityHandle = typename TArchetypeList::Handle;

            bool IsValid(TEntityHandle handle) const
            {
                const TArchetypeList* list = FindOwningArchetypeList(handle);
                return list && list->IsValid(handle);
            }

            bool RegisterArchetypeDefinition(const TArchetypeDefinition& definition)
            {
                if (ArchetypeDefinitions.IsFull())
                    return false;

                return ArchetypeDefinitions.Insert(definition.GetId(), definition);
            }

            bool UnregisterArchetypeDefinition(const TArchetypeDefinition& definition)
            {
                // TODO (jfarris): what should we do with existing archetype lists?
                return ArchetypeDefinitions.Remove(definition.GetId());
            }

            bool HasArchetypeDefinition(const FName& name) const
            {
                return ArchetypeDefinitions.Contains(name);
            }

            // Acquire an archetype for a given entity
            TEntityHandle Acquire(EntityId entityId, const FName& archetypeId)
            {
                TArchetypeList* list = FindOrAddArchetypeList(archetypeId);
                if (!list)
                {
                    return TEntityHandle();
                }

                return list->Allocate(entityId);
            }

            // Release an archetype back to the pool
            bool Release(TEntityHandle handle)
            {
                TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list)
                {
                    return false;
                }

                // TODO (jfarris): release chunk when list becomes empty?
                return list->Deallocate(handle);
            }

            bool SetArchetype(const TEntityHandle& handle, const FName& archetypeId)
            {
                // TODO (jfarris): implement
                return false;
            }

            void* AddComponent(const TEntityHandle& handle, const FName& componentId)
            {
                // TODO (jfarris): implement
                return nullptr;
            }

            template <class T>
            T* AddComponent(const TEntityHandle& handle, const T& defaultValue = {})
            {
                // TODO (jfarris): implement
                return nullptr;
            }

            template <class T, class ...TArgs>
            T* EmplaceComponent(const TEntityHandle& handle, const TArgs&...args)
            {
                // TODO (jfarris): implement
                return nullptr;
            }

            bool RemoveComponent(const TEntityHandle& handle, const FName& componentId)
            {
                // TODO (jfarris): implement
                return false;
            }

            template <class T>
            bool RemoveComponent(const TEntityHandle& handle)
            {
                // TODO (jfarris): implement
                return false;
            }

            uint32 RemoveAllComponents(const TEntityHandle& handle)
            {
                // TODO (jfarris): implement
                return 0;
            }

            void* GetComponentPtr(const TEntityHandle& handle, const FName& componentId)
            {
                TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list || !list->IsValid(handle))
                {
                    return nullptr;
                }

                return list->GetComponentPtr(handle, componentId);
            }

            const void* GetComponentPtr(const TEntityHandle& handle, const FName& componentId) const
            {
                const TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list || !list->IsValid(handle))
                {
                    return nullptr;
                }

                return list->GetComponentPtr(handle, componentId);
            }

            template <class T>
            T* GetComponentPtr(const TEntityHandle& handle)
            {
                TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list || !list->IsValid(handle))
                {
                    return nullptr;
                }

                return list->template GetComponentPtr<T>(handle);
            }

            template <class T>
            const T* GetComponentPtr(const TEntityHandle& handle) const
            {
                const TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list || !list->IsValid(handle))
                {
                    return nullptr;
                }

                return list->template GetComponentPtr<T>(handle);
            }

            TArchetypeList* FindFirstArchetypeList(const FName& archetypeId, bool includeFullLists = false)
            {
                for (uint32 i = 0; i < ChunkAllocator.NumChunks; ++i)
                {
                    if ((FName)(ChunkAllocator.Chunks[i].UserData) == archetypeId)
                    {
                        TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle{i});
                        if (list && (includeFullLists || !list->IsFull()))
                        {
                            return list;
                        }
                    }
                }
                return nullptr;
            }

            TArchetypeList* FindOwningArchetypeList(const TEntityHandle& handle)
            {
                TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle { handle.GetOwnerId() });
                return list && list->IsValid(handle) ? list : nullptr;
            }

            const TArchetypeList* FindOwningArchetypeList(const TEntityHandle& handle) const
            {
                const TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle { handle.GetOwnerId() });
                return list && list->IsValid(handle) ? list : nullptr;
            }

            void ForEachArchetypeList(const FName& archetypeId, const TFunction<void(TArchetypeList&)>& func)
            {
                for (uint32 i = 0; i < ChunkAllocator.NumChunks; ++i)
                {
                    FName archetypeListId = ChunkAllocator.Chunks[i].UserData;
                    if (archetypeListId == archetypeId)
                    {
                        TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle{i});
                        if (list)
                        {
                            func(*list);
                        }
                    }
                }
            }

            template <class ...TComponents>
            void ForEachEntity(const EntityQuery& query, const TEntityQueryWorkFunc<TComponents...>& func)
            {
                for (uint32 i = 0; i < ChunkAllocator.NumChunks; ++i)
                {
                    TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle{i});
                    if (list && query.PassesFilter(list->GetDefinition()))
                    {
                        list->template ForEachEntity<TComponents...>([&](EntityId entityId, TComponents ...components)
                        {
                            func(entityId, Forward<TComponents>(components)...);
                        });
                    }
                }
            }

            template <class ...TComponents>
            void ForEachEntity(const EntityQuery& query, const TEntityQueryWorkBufferFunc<TComponents...>& func)
            {
                for (uint32 i = 0; i < ChunkAllocator.NumChunks; ++i)
                {
                    TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle{i});
                    if (list && query.PassesFilter(list->GetDefinition()))
                    {
                        func(EntityComponentSpan<TComponents...>::FromList(*list));
                    }
                }
            }

            template <class T>
            void ForEachComponent(const TEntityHandle& handle, const T& callback)
            {
                TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list)
                {
                    return;
                }

                list->ForEachComponent(handle, callback);
            }

            template <class T>
            void ForEachComponent(const TEntityHandle& handle, const T& callback) const
            {
                const TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list)
                {
                    return;
                }

                list->ForEachComponent(handle, callback);
            }

            EntityQueryBuilder<TArchetypeManager> Entities()
            {
                return EntityQueryBuilder<TArchetypeManager>(this);
            }

            template <class ...TComponents>
            void Schedule(const EntityQuery& query, const TEntityQueryWorkFunc<TComponents...>& func)
            {
                PHX_ASSERT(!Queries.IsFull());

                for (uint32 i = 0; i < ChunkAllocator.NumChunks; ++i)
                {
                    TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle{i});
                    if (list && query.PassesFilter(list->GetDefinition()))
                    {
                        TEntityQueryWorker<TArchetypeManager, TComponents...> worker(list, func);
                        Queries.Allocate(worker);
                    }
                }
            }

            template <class ...TComponents>
            void Schedule(const EntityQuery& query, const TEntityQueryWorkBufferFunc<TComponents...>& func)
            {
                PHX_ASSERT(!Queries.IsFull());

                for (uint32 i = 0; i < ChunkAllocator.NumChunks; ++i)
                {
                    TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle{i});
                    if (list && query.PassesFilter(list->GetDefinition()))
                    {
                        TEntityQueryBufferWorker<TArchetypeManager, TComponents...> worker(list, func);
                        Queries.Allocate(worker);
                    }
                }
            }

            template <class ...TComponents>
            void ScheduleParallel(const EntityQuery& query, const TEntityQueryWorkFunc<TComponents...>& func)
            {
                PHX_ASSERT(!Queries.IsFull());

                for (uint32 i = 0; i < ChunkAllocator.NumChunks; ++i)
                {
                    TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle{i});
                    if (list && query.PassesFilter(list->GetDefinition()))
                    {
                        TEntityQueryWorker<TArchetypeManager, TComponents...> worker(list, func);
                        Queries.Allocate(worker);
                    }
                }
            }

            template <class ...TComponents>
            void ScheduleParallel(const EntityQuery& query, const TEntityQueryWorkBufferFunc<TComponents...>& func)
            {
                PHX_ASSERT(!Queries.IsFull());

                for (uint32 i = 0; i < ChunkAllocator.NumChunks; ++i)
                {
                    TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle{i});
                    if (list && query.PassesFilter(list->GetDefinition()))
                    {
                        TEntityQueryBufferWorker<TArchetypeManager, TComponents...> worker(list, func);
                        Queries.Allocate(worker);
                    }
                }
            }

            void ProcessQueries(WorldRef world)
            {
                for (auto && [handle, data] : Queries)
                {
                    auto worker = static_cast<IEntityQueryWorker<TArchetypeManager>*>(data);
                    worker->Execute(world, *this);
                }
                Queries.Reset();
            }

        //private:

            TArchetypeList* FindOrAddArchetypeList(const FName& archetypeId)
            {
                TArchetypeList* list = FindFirstArchetypeList(archetypeId, false);
                if (list)
                {
                    return list;
                }
                
                TArchetypeDefinition* archetypeDef = ArchetypeDefinitions.GetPtr(archetypeId);
                if (!archetypeDef)
                {
                    return nullptr;
                }
                
                TChunkHandle handle = ChunkAllocator.template Allocate<TArchetypeList>((uint32)archetypeId, *archetypeDef);
                list = ChunkAllocator.template GetPtr<TArchetypeList>(handle);
                list->SetId(handle.Id);

                return list;
            }

            TFixedMap<FName, TArchetypeDefinition, MaxNumArchetypes> ArchetypeDefinitions;
            TFixedArena<1024> Queries; 
            TChunkAllocator ChunkAllocator;
        };
        
        PHOENIXECS_API void Test(Session* session);
    }
}
