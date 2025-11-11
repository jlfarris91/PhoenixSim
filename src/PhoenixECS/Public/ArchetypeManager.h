
#pragma once

#include "ArchetypeDefinition.h"
#include "ArchetypeList.h"
#include "EntityQuery.h"
#include "Platform.h"
#include "Containers/FixedMap.h"
#include "Name.h"
#include "Session.h"
#include "Containers/FixedBlockAllocator.h"
#include "Utils.h"
#include "Containers/FixedArena.h"

namespace Phoenix
{
    namespace ECS
    {
        template <
            class TArchetypeDefinition = TArchetypeDefinition<8>,
            uint16 MaxNumComponents = 128,
            uint16 MaxNumArchetypes = 32,
            uint16 MaxNumArchetypeLists = 1024,
            uint32 ArchetypeListSize = 16000
        >
        class TArchetypeManager
        {
        public:

            using TArchetypeList = TArchetypeList<TArchetypeDefinition, ArchetypeListSize>;
            using TArchetypeLists = TFixedBlockAllocator<MaxNumArchetypeLists, sizeof(TArchetypeList)>;
            using TBlockHandle = typename TArchetypeLists::Handle;
            using TEntityHandle = typename TArchetypeList::Handle;
            using TComponentDefMap = TFixedMap<FName, ComponentDefinition, MaxNumComponents>;
            using TArchetypeDefMap = TFixedMap<FName, TArchetypeDefinition, MaxNumArchetypes>;

            bool IsValid(TEntityHandle handle) const
            {
                const TArchetypeList* list = FindOwningArchetypeList(handle);
                return list && list->IsValid(handle);
            }

            size_t GetNumActiveArchetypes() const
            {
                size_t total = 0;
                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    if (const TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle))
                    {
                        total += list->GetNumActiveInstances();
                    }
                }
                return total;
            }

            size_t GetNumArchetypeLists() const
            {
                return ArchetypeLists.GetNumOccupiedBlocks();
            }

            //
            // Archetype Definitions
            //

            const TArchetypeDefMap& GetArchetypeDefinitions() const
            {
                return ArchetypeDefinitions;
            }

            bool RegisterArchetypeDefinition(const TArchetypeDefinition& definition)
            {
                if (ArchetypeDefinitions.Contains(definition.GetArchetypeHash()))
                {
                    return true;
                }

                if (FName::IsNoneOrEmpty(definition.GetArchetypeHash()))
                {
                    return false;
                }

                if (ArchetypeDefinitions.IsFull())
                {
                    return false;
                }

                for (uint16 i = 0; i < definition.GetNumComponents(); ++i)
                {
                    if (!RegisterComponentDefinition(definition[i]))
                    {
                        return false;
                    }
                }

                return ArchetypeDefinitions.Insert(definition.GetArchetypeHash(), definition);
            }

            bool UnregisterArchetypeDefinition(const TArchetypeDefinition& definition)
            {
                // TODO (jfarris): what should we do with existing archetype lists?
                return ArchetypeDefinitions.Remove(definition.GetArchetypeHash());
            }

            bool IsArchetypeRegistered(const FName& archetypeIdOrHash) const
            {
                return GetArchetypeDefinition(archetypeIdOrHash) != nullptr;
            }

            FName GetArchetypeHash(const FName& archetypeId) const
            {
                const TArchetypeDefinition* archDefPtr = GetArchetypeDefinitionById(archetypeId);
                return archDefPtr ? archDefPtr->GetArchetypeHash() : FName::None;
            }

            const TArchetypeDefinition* GetArchetypeDefinitionByHash(const FName& archetypeHash) const
            {
                return ArchetypeDefinitions.GetPtr(archetypeHash);
            }

            const TArchetypeDefinition* GetArchetypeDefinitionById(const FName& archetypeId) const
            {
                for (auto && [compHash, archDef] : ArchetypeDefinitions)
                {
                    if (archDef.GetId() == archetypeId)
                        return &archDef;
                }
                return nullptr;
            }

            const TArchetypeDefinition* GetArchetypeDefinition(const FName& archetypeIdOrHash) const
            {
                if (const TArchetypeDefinition* archDefPtr = GetArchetypeDefinitionByHash(archetypeIdOrHash))
                {
                    return archDefPtr;
                }
                return GetArchetypeDefinitionById(archetypeIdOrHash);
            }

            //
            // Component Definitions
            //

            const TComponentDefMap& GetComponentDefinitions() const
            {
                return ComponentDefinitions;
            }

            bool RegisterComponentDefinition(const ComponentDefinition& definition)
            {
                if (IsComponentRegistered(definition.Id))
                    return true;

                if (ComponentDefinitions.IsFull())
                    return false;

                return ComponentDefinitions.Insert(definition.Id, definition);
            }

            bool UnregisterComponentDefinition(const ComponentDefinition& definition)
            {
                // TODO (jfarris): what should we do with existing archetype definitions containing the component?
                return ComponentDefinitions.Remove(definition.Id);
            }

            bool IsComponentRegistered(const FName& componentId) const
            {
                return ComponentDefinitions.Contains(componentId);
            }

            // Acquire an archetype for a given entity
            TEntityHandle Acquire(EntityId entityId, const FName& archetypeIdOrHash)
            {
                TArchetypeList* list = FindOrAddArchetypeList(archetypeIdOrHash);
                if (!list)
                {
                    return TEntityHandle();
                }

                return list->Acquire(entityId);
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
                return list->Release(handle);
            }

            // Sets the archetype of a given entity to a new archetype.
            // Data for any components shared between the current archetype and the new archetype are preserved.
            TEntityHandle SetArchetype(const TEntityHandle& handle, const FName& archetypeIdOrHash)
            {
                typename TArchetypeList::Handle newHandle(handle.GetEntityId());

                TArchetypeList* currList = FindOwningArchetypeList(handle);

                // The entity is already in a list with the given archetype id. Nothing changes.
                if (currList && currList->HasArchetypeDefinition(archetypeIdOrHash))
                {
                    return handle;
                }

                // Allocate a new archetype for the entity in a new list.
                if (TArchetypeList* newList = FindOrAddArchetypeList(archetypeIdOrHash))
                {
                    newHandle = newList->Acquire(handle.GetEntityId());

                    // Copy over any component data to the new archetype
                    if (currList)
                    {
                        currList->ForEachComponent(handle, [&](const ComponentDefinition& compDef, const void* currComp)
                        {
                            if (void* newComp = newList->GetComponent(newHandle, compDef.Id))
                            {
                                memcpy(newComp, currComp, compDef.Size);
                            }
                        });
                    }
                }
                
                // Try to remove the entity from the list it currently belongs to.
                if (currList)
                {
                    currList->Release(handle);
                }

                return newHandle;
            }

            void* AddComponent(TEntityHandle& inOutHandle, const ComponentDefinition& componentDef)
            {
                TArchetypeDefinition currArchDef;

                if (const TArchetypeList* currList = FindOwningArchetypeList(inOutHandle))
                {
                    currArchDef = currList->GetDefinition();
                }

                TArchetypeDefinition newArchDef;
                if (!TArchetypeDefinition::AddComponent(currArchDef, componentDef, newArchDef))
                {
                    // Failed to add the component to the archetype. Could be that the current archetype definition
                    // already had that component or it can't add any more components.
                    return nullptr;
                }

                TArchetypeDefinition* archDef = ArchetypeDefinitions.FindOrAdd(newArchDef.GetArchetypeHash(), newArchDef);
                if (!archDef)
                {
                    // Failed to add the new archetype, probably out of space.
                    return nullptr;
                }

                inOutHandle = SetArchetype(inOutHandle, archDef->GetArchetypeHash());

                return GetComponent(inOutHandle, componentDef.Id);
            }

            void* AddComponent(TEntityHandle& inOutHandle, const FName& componentId)
            {
                const ComponentDefinition* compDef = ComponentDefinitions.GetPtr(componentId);
                if (!compDef)
                {
                    return nullptr;
                }

                return AddComponent(inOutHandle, *compDef);
            }

            template <class T>
            T* AddComponent(TEntityHandle& inOutHandle, const T& defaultValue = {})
            {
                ComponentDefinition compDef = ComponentDefinition::Create<T>();
                T* compPtr = static_cast<T*>(AddComponent(inOutHandle, compDef));
                if (!compPtr)
                {
                    return nullptr;
                }
                *compPtr = defaultValue;
                return compPtr;
            }

            template <class T, class ...TArgs>
            T* EmplaceComponent(TEntityHandle& inOutHandle, const TArgs&...args)
            {
                ComponentDefinition compDef = ComponentDefinition::Create<T>();
                T* compPtr = static_cast<T*>(AddComponent(inOutHandle, compDef));
                if (!compPtr)
                {
                    return nullptr;
                }
                new (compPtr) T(args...);
                return compPtr;
            }

            bool RemoveComponent(TEntityHandle& inOutHandle, const FName& componentId)
            {
                TArchetypeDefinition currArchDef;

                const TArchetypeList* currList = FindOwningArchetypeList(inOutHandle);
                if (!currList)
                {
                    return false;
                }

                TArchetypeDefinition newArchDef;
                if (!TArchetypeDefinition::RemoveComponent(currArchDef, componentId, newArchDef))
                {
                    // Failed to remove the component from the archetype.
                    // The current archetype definition must not have the component.
                    return false;
                }

                TArchetypeDefinition* archDef = ArchetypeDefinitions.FindOrAdd(newArchDef.GetArchetypeHash(), newArchDef);
                if (!archDef)
                {
                    // Failed to add the new archetype, probably out of space.
                    return false;
                }

                inOutHandle = SetArchetype(inOutHandle, archDef->GetArchetypeHash());
                
                return true;
            }

            template <class T>
            bool RemoveComponent(TEntityHandle& inOutHandle)
            {
                return RemoveComponent(inOutHandle, T::StaticTypeName);
            }

            bool RemoveAllComponents(TEntityHandle& inOutHandle)
            {
                TArchetypeList* currList = FindOwningArchetypeList(inOutHandle);
                return currList && currList->Release(inOutHandle);
            }

            void* GetComponent(const TEntityHandle& handle, const FName& componentId)
            {
                TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list || !list->IsValid(handle))
                {
                    return nullptr;
                }

                return list->GetComponent(handle, componentId);
            }

            const void* GetComponent(const TEntityHandle& handle, const FName& componentId) const
            {
                const TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list || !list->IsValid(handle))
                {
                    return nullptr;
                }

                return list->GetComponent(handle, componentId);
            }

            template <class T>
            T* GetComponent(const TEntityHandle& handle)
            {
                TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list || !list->IsValid(handle))
                {
                    return nullptr;
                }

                return list->template GetComponent<T>(handle);
            }

            template <class T>
            const T* GetComponent(const TEntityHandle& handle) const
            {
                const TArchetypeList* list = FindOwningArchetypeList(handle);
                if (!list || !list->IsValid(handle))
                {
                    return nullptr;
                }

                return list->template GetComponent<T>(handle);
            }

            TArchetypeList* FindFirstArchetypeList(const FName& archetypeIdOrHash, bool includeFullLists = false)
            {
                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
                    if (list && list->HasArchetypeDefinition(archetypeIdOrHash) && (includeFullLists || !list->IsFull()))
                    {
                        return list;
                    }
                }
                return nullptr;
            }

            TArchetypeList* FindOwningArchetypeList(const TEntityHandle& handle)
            {
                TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(TBlockHandle { handle.GetOwnerId() });
                return list && list->IsValid(handle) ? list : nullptr;
            }

            const TArchetypeList* FindOwningArchetypeList(const TEntityHandle& handle) const
            {
                const TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(TBlockHandle { handle.GetOwnerId() });
                return list && list->IsValid(handle) ? list : nullptr;
            }

            void ForEachArchetypeList(const TFunction<void(TArchetypeList&)>& func)
            {
                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    const TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
                    if (list)
                    {
                        func(*list);
                    }
                }
            }

            void ForEachArchetypeList(const TFunction<void(const TArchetypeList&)>& func) const
            {
                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    const TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
                    if (list)
                    {
                        func(*list);
                    }
                }
            }

            void ForEachArchetypeList(const FName& archetypeIdOrHash, const TFunction<void(TArchetypeList&)>& func)
            {
                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
                    if (list && list->HasArchetypeDefinition(archetypeIdOrHash))
                    {
                        func(*list);
                    }
                }
            }

            template <class ...TComponents>
            void ForEachEntity(const EntityQuery& query, const TEntityQueryFunc<TComponents...>& func)
            {
                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
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
            void ForEachEntity(const EntityQuery& query, const TEntityQueryBufferFunc<TComponents...>& func)
            {
                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
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

            EntityQueryBuilder<TArchetypeManager> Entities() const
            {
                return EntityQueryBuilder<TArchetypeManager>(const_cast<TArchetypeManager*>(this));
            }

            template <class ...TComponents>
            void Schedule(const EntityQuery& query, const TEntityQueryJobFunc<TComponents...>& func)
            {
                PHX_ASSERT(!Queries.IsFull());

                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
                    if (list && query.PassesFilter(list->GetDefinition()))
                    {
                        TEntityQueryWorker<TArchetypeManager, TComponents...> worker(list, func);
                        Queries.Allocate(worker);
                    }
                }
            }

            template <class ...TComponents>
            void Schedule(const EntityQuery& query, const TEntityQueryBufferJobFunc<TComponents...>& func)
            {
                PHX_ASSERT(!Queries.IsFull());

                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
                    if (list && query.PassesFilter(list->GetDefinition()))
                    {
                        TEntityQueryBufferWorker<TArchetypeManager, TComponents...> worker(list, func);
                        Queries.Allocate(worker);
                    }
                }
            }

            template <class ...TComponents>
            void ScheduleParallel(const EntityQuery& query, const TEntityQueryJobFunc<TComponents...>& func)
            {
                PHX_ASSERT(!Queries.IsFull());

                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
                    if (list && query.PassesFilter(list->GetDefinition()))
                    {
                        TEntityQueryWorker<TArchetypeManager, TComponents...> worker(list, func);
                        Queries.Allocate(worker);
                    }
                }
            }

            template <class ...TComponents>
            void ScheduleParallel(const EntityQuery& query, const TEntityQueryBufferJobFunc<TComponents...>& func)
            {
                PHX_ASSERT(!Queries.IsFull());

                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
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

            void Compact()
            {
                // Free any archetype lists with no active instances.
                for (const TBlockHandle& handle : ArchetypeLists)
                {
                    TArchetypeList* list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
                    if (list && list->GetNumActiveInstances() == 0)
                    {
                        ArchetypeLists.Deallocate(handle);
                    }
                }

                ArchetypeLists.Compact();
            }

        private:

            TArchetypeList* FindOrAddArchetypeList(const FName& archetypeIdOrHash)
            {
                TArchetypeList* list = FindFirstArchetypeList(archetypeIdOrHash, false);
                if (list)
                {
                    return list;
                }
                
                const TArchetypeDefinition* archetypeDef = GetArchetypeDefinition(archetypeIdOrHash);
                if (!archetypeDef)
                {
                    return nullptr;
                }
                
                TBlockHandle handle = ArchetypeLists.template Allocate<TArchetypeList>(archetypeDef->GetArchetypeHash(), *archetypeDef);
                list = ArchetypeLists.template GetPtr<TArchetypeList>(handle);
                list->SetId(handle.Id);

                return list;
            }

            // A mapping of component id to component definition.
            TComponentDefMap ComponentDefinitions;

            // A mapping of archetype hash to archetype definition.
            TArchetypeDefMap ArchetypeDefinitions;

            TArchetypeLists ArchetypeLists;

            TFixedArena<1024> Queries;
        };
    }
}
