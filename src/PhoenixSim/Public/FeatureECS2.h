
#pragma once

#include "DLLExport.h"
#include "FeatureECS.h"
#include "Platform.h"
#include "Containers/FixedMap.h"
#include "Hashing.h"
#include "Name.h"
#include "Containers/FixedChunkAllocator.h"

namespace Phoenix
{
    namespace ECS2
    {
        struct ComponentDefinition
        {
            FName Id = 0;
            uint32 Size = 0;
            uint32 Offset = 0;
        };

        template <uint8 MaxComponents>
        struct TArchetypeDefinition
        {
            TArchetypeDefinition() = default;

            TArchetypeDefinition(ComponentDefinition* comps, uint8 n)
            {
                PHX_ASSERT(n <= MaxComponents);

                TotalSize = 0;
                Id = 0;
                for (uint8 i = 0; i < n; ++i)
                {
                    ComponentDefinition& def = Components[i];
                    def = comps[i];
                    def.Offset = TotalSize;

                    TotalSize += def.Size;
                    Id += def.Id;
                }

                NumComponents = n;
            }

            uint16 GetNumComponents() const
            {
                return NumComponents;
            }

            uint16 GetTotalSize() const
            {
                return TotalSize;
            }

            const ComponentDefinition& operator[](uint32 index) const
            {
                PHX_ASSERT(index < MaxComponents);
                return Components[index];
            }

            uint16 FindComponentDefinition(const FName& componentId)
            {
                for (uint32 i = 0; i < NumComponents; ++i)
                {
                    if (Components[i].Id == componentId)
                        return i;
                }
                return -1;
            }

            void Construct(void* data)
            {
                // TODO (construct components):
                memset(data, 0, TotalSize);
            }

            void Deconstruct(void* data)
            {
                // TODO (destruct components):
                memset(data, 0, TotalSize);
            }

            FName Id = 0;
            ComponentDefinition Components[MaxComponents] = {};
            uint8 NumComponents = 0;
            uint16 TotalSize = 0;
        };

        template <class TArchetypeDefinition, uint32 N>
        struct TArchetypeList
        {
            struct Handle
            {
                uint32 Id = -1;
            };

            struct ArchetypeInstance
            {
                uint32 Id = 0;
            };

            TArchetypeList(const TArchetypeDefinition& definition, const FName& id)
                : Id(id)
                , Definition(definition)
            {
            }

            FName GetId() const
            {
                return Id;
            }

            const TArchetypeDefinition& GetDefinition() const
            {
                return Definition;
            }

            constexpr bool IsFull() const
            {
                return Size + Definition.TotalSize >= N;
            }

            constexpr bool IsValid(Handle entityHandle) const
            {
                if (entityHandle.Id >= HighWaterMark)
                {
                    return false;
                }

                const ArchetypeInstance* instance = GetEntityPtr(entityHandle);
                return instance && instance->Id == entityHandle.Id;
            }

            Handle Allocate()
            {
                if (IsFull())
                {
                    return Handle();
                }

                Handle entityHandle = { HighWaterMark++ };
                ArchetypeInstance* instance = GetEntityPtr(entityHandle);
                instance->Id = entityHandle.Id;

                Definition.Construct(GetEntityComponentDataPtr(entityHandle));

                ++NumEntities;

                return entityHandle;
            }

            bool Deallocate(Handle entityHandle)
            {
                if (!IsValid(entityHandle))
                {
                    return false;
                }

                ArchetypeInstance* instance = GetEntityPtr(entityHandle);
                instance->Id = -1;

                Definition.Deconstruct(GetEntityComponentDataPtr(entityHandle));

                --NumEntities;

                return entityHandle;
            }

            uint8* GetComponentData(Handle entityHandle, const FName& componentId)
            {
                auto entityCompOffset = GetEntityComponentOffset(entityHandle, componentId);
                PHX_ASSERT(entityCompOffset < N);
                return Data + entityCompOffset;
            }

            const uint8* GetComponentData(Handle entityHandle, const FName& componentId) const
            {
                auto entityCompOffset = GetEntityComponentOffset(entityHandle, componentId);
                PHX_ASSERT(entityCompOffset < N);
                return Data + entityCompOffset;
            }

            template <class T>
            T* GetComponent(Handle entityHandle)
            {
                uint8* dataPtr = GetComponentData(entityHandle, T::StaticName);
                return reinterpret_cast<T*>(dataPtr);
            }

            template <class T>
            const T* GetComponent(Handle entityHandle) const
            {
                const uint8* dataPtr = GetComponentData(entityHandle, T::StaticName);
                return reinterpret_cast<const T*>(dataPtr);
            }

        private:

            constexpr uint16 GetEntityOffset(Handle entityHandle) const
            {
                return entityHandle.Id * (sizeof(ArchetypeInstance) + Definition.GetTotalSize());
            }

            constexpr uint16 GetComponentOffset(const FName& componentId) const
            {
                uint16 index = Definition.FindComponentDefinition(componentId);
                if (index == Index<uint16>::None)
                {
                    return Index<uint16>::None;
                }

                const ComponentDefinition& def = Definition[index];
                return def.Offset;
            }

            constexpr uint16 GetEntityComponentOffset(Handle entityHandle, const FName& componentId) const
            {
                uint16 componentOffset = GetComponentOffset(componentId);
                PHX_ASSERT(componentOffset != Index<uint16>::None);
                return GetEntityOffset(entityHandle) + componentOffset;
            }

            constexpr ArchetypeInstance* GetEntityPtr(Handle entityHandle)
            {
                uint16 offset = GetEntityOffset(entityHandle);
                return reinterpret_cast<ArchetypeInstance*>(Data + offset);
            }

            constexpr const ArchetypeInstance* GetEntityPtr(Handle entityHandle) const
            {
                uint16 offset = GetEntityOffset(entityHandle);
                return reinterpret_cast<const ArchetypeInstance*>(Data + offset);
            }

            constexpr ArchetypeInstance* GetEntityComponentDataPtr(Handle entityHandle)
            {
                uint16 offset = GetEntityOffset(entityHandle) + sizeof(ArchetypeInstance);
                return reinterpret_cast<ArchetypeInstance*>(Data + offset);
            }

            constexpr const ArchetypeInstance* GetEntityComponentDataPtr(Handle entityHandle) const
            {
                uint16 offset = GetEntityOffset(entityHandle) + sizeof(ArchetypeInstance);
                return reinterpret_cast<const ArchetypeInstance*>(Data + offset);
            }

            constexpr uint8* GetEntityComponentPtr(Handle entityHandle, const FName& componentId)
            {
                uint16 offset = GetEntityComponentOffset(entityHandle, componentId);
                return Data + offset;
            }

            constexpr const uint8* GetEntityComponentPtr(Handle entityHandle, const FName& componentId) const
            {
                uint16 offset = GetEntityComponentOffset(entityHandle, componentId);
                return Data + offset;
            }

            FName Id = 0;
            TArchetypeDefinition Definition;
            uint16 Size = 0;
            uint16 NumEntities = 0;
            uint16 HighWaterMark = 0;
            uint8 Data[N] = {};
        };

        template <
            class TArchetypeDefinition,
            uint16 MaxNumArchetypes,
            uint16 MaxNumArchetypeLists,
            uint32 ArchetypeListSize
        >
        struct TEntityManager
        {
            using TArchetypeList = TArchetypeList<TArchetypeDefinition, ArchetypeListSize>;
            using TChunkAllocator = TFixedChunkAllocator<MaxNumArchetypeLists, sizeof(TArchetypeList)>;
            using TChunkHandle = typename TChunkAllocator::Handle;
            using TEntityHandle = typename TArchetypeList::Handle;

            bool IsValid(TEntityHandle handle) const
            {
                for (uint32 i = 0; i < ChunkAllocator.NumChunks; ++i)
                {
                    auto list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle { i });
                    if (list && list->IsValid(handle))
                        return true;
                }
                return false;
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

            TEntityHandle AcquireEntity(const FName& archetypeId)
            {
                TArchetypeList* list = FindOrAddArchetypeList(archetypeId);
                if (!list)
                {
                    return TEntityHandle();
                }

                return list->Allocate();
            }

            TArchetypeList* FindArchetypeList(const FName& archetypeId)
            {
                for (uint32 i = 0; i < ChunkAllocator.NumChunks; ++i)
                {
                    if ((FName)(ChunkAllocator.Chunks[i].UserData) == archetypeId)
                    {
                        TArchetypeList* list = ChunkAllocator.template GetPtr<TArchetypeList>(TChunkHandle(i));
                        if (list && !list->IsFull())
                            return list;
                    }
                }
                return nullptr;
            }

            TArchetypeList* FindOrAddArchetypeList(const FName& archetypeId)
            {
                TArchetypeList* list = FindArchetypeList(archetypeId);
                if (list)
                {
                    return list;
                }
                
                TArchetypeDefinition* archetypeDef = ArchetypeDefinitions.GetPtr(archetypeId);
                if (!archetypeDef)
                {
                    return nullptr;
                }
                
                TChunkHandle handle = ChunkAllocator.template Allocate<TArchetypeList>((uint32)archetypeId, *archetypeDef, archetypeId);
                list = ChunkAllocator.template GetPtr<TArchetypeList>(handle);

                return list;
            }

            TFixedMap<FName, TArchetypeDefinition, MaxNumArchetypes> ArchetypeDefinitions;
            TChunkAllocator ChunkAllocator;
        };

        template <class TEntityManager, class ...TComponents>
        struct TypedArchetypeIter
        {
            using Tuple = TTuple<TComponents...>;

            struct EntityComponents
            {
                ECS::EntityId EntityId;
                TTuple<TComponents...> Components;
            };

            TypedArchetypeIter(TEntityManager* entityManager, uint16 chunkIndex, uint16 entityIndex)
                : EM(entityManager)
                , ChunkIndex(chunkIndex)
                , EntityIndex(entityIndex)
                , CurrList(nullptr)
            {
            }

            const EntityComponents& operator*() const
            {
                return Current;
            }

            TypedArchetypeIter& operator++()
            {
                return *this;
            }

        private:
            TEntityManager* EM;
            EntityComponents Current;
            uint16 ChunkIndex;
            uint16 EntityIndex;
            typename TEntityManager::TArchetypeList* CurrList;
        };

        PHOENIXSIM_API void Test();
    }
}
