
#pragma once

#include "ArchetypeHandle.h"
#include "ArchetypeDefinition.h"
#include "EntityId.h"
#include "Name.h"
#include "Platform.h"

#ifndef PHX_ECS_ARCHETYPE_LIST_SIZE
#define PHX_ECS_ARCHETYPE_LIST_SIZE 16000
#endif

namespace Phoenix
{
    namespace ECS
    {
        template <class>
        struct ComponentAccessor
        {
        };

        template <class TComp>
        struct ComponentAccessor<const TComp&>
        {
            using TCompUnderlying = Underlying_T<TComp>;
            static const TComp& GetComponentRef(TCompUnderlying* data)
            {
                return *data;
            }
        };

        template <class TComp>
        struct ComponentAccessor<TComp&>
        {
            using TCompUnderlying = Underlying_T<TComp>;
            static TComp& GetComponentRef(TCompUnderlying* data)
            {
                return *data;
            }
        };

        struct ArchetypeInstance
        {
            EntityId EntityId;
            uint32 NextFree = Index<uint32>::None;
        };

        // Archetype data is tightly packed into the Data buffer.
        // Data: [Entity0][Comp0][Comp1][Entity1][Comp0][Comp1]...
        template <class TArchetypeDefinition = ArchetypeDefinition, uint32 N = PHX_ECS_ARCHETYPE_LIST_SIZE>
        class TArchetypeList
        {
        public:

            static constexpr size_t Capacity = N;

            using Handle = ArchetypeHandle;

            TArchetypeList(const TArchetypeDefinition& definition, uint32 id = 0)
                : Id(id)
                , Definition(definition)
            {
            }

            uint32 GetId() const
            {
                return Id;
            }

            void SetId(uint32 id)
            {
                Id = id;
            }

            const TArchetypeDefinition& GetDefinition() const
            {
                return Definition;
            }

            bool HasArchetypeDefinition(const FName& archetypeIdOrHash) const
            {
                return Definition.HasIdOrHash(archetypeIdOrHash);
            }

            constexpr uint8* GetData()
            {
                return Data;
            }

            constexpr uint8* GetData() const
            {
                return Data;
            }

            constexpr uint32 GetSize() const
            {
                return NumInstances * GetEntityTotalSize();
            }

            constexpr bool IsFull() const
            {
                return NumActiveInstances == GetInstanceCapacity();
            }

            constexpr uint32 GetNumInstances() const
            {
                return NumInstances;
            }

            constexpr uint32 GetNumActiveInstances() const
            {
                return NumActiveInstances;
            }

            constexpr uint32 GetInstanceCapacity() const
            {
                return Capacity / GetEntityTotalSize();
            }

            constexpr bool IsValid(const Handle& handle) const
            {
                if (!OwnsHandle(handle) || handle.Id >= NumInstances)
                {
                    return false;
                }

                const ArchetypeInstance* instance = GetEntityPtr(handle.Id);
                return instance && instance->EntityId == handle.EntityId;
            }

            constexpr bool OwnsHandle(const Handle& handle) const
            {
                return handle.OwnerId == Id;
            }

            Handle Acquire(EntityId entityId)
            {
                if (IsFull())
                {
                    return Handle();
                }

                uint32 slotIndex = FindFreeSlot();
                if (slotIndex == Index<uint32>::None)
                {
                    slotIndex = NumInstances++;
                }
                
                Handle handle = { Id, slotIndex, entityId };
                ArchetypeInstance* instance = GetEntityPtr(handle.Id);
                instance->EntityId = entityId;

                void* entityComponentDataPtr = GetEntityComponentHeadPtr(handle.Id);
                PHX_ASSERT(entityComponentDataPtr);

                Definition.Construct(entityComponentDataPtr);

                ++NumActiveInstances;

                return handle;
            }

            bool Release(const Handle& handle)
            {
                if (!OwnsHandle(handle))
                {
                    return false;
                }

                ArchetypeInstance* instance = GetEntityPtr(handle.Id);
                if (!instance || instance->EntityId != handle.EntityId)
                {
                    return false;
                }

                instance->EntityId = EntityId::Invalid;

                if (FreeHead == Index<uint32>::None)
                {
                    FreeHead = FreeTail = handle.Id;
                }
                else if (ArchetypeInstance* freeTail = GetEntityPtr(FreeTail))
                {
                    freeTail->NextFree = handle.Id;
                    FreeTail = handle.Id;
                }

                void* entityComponentDataPtr = GetEntityComponentHeadPtr(handle.Id);
                PHX_ASSERT(entityComponentDataPtr);

                Definition.Deconstruct(entityComponentDataPtr);

                --NumActiveInstances;

                return true;
            }

            constexpr void* GetComponent(const Handle& handle, const FName& componentId)
            {
                if (!IsValid(handle))
                {
                    return nullptr;
                }
                uint32 entityCompOffset = GetOffsetToEntityComponent(handle.Id, componentId);
                if (entityCompOffset == Index<uint32>::None)
                {
                    return nullptr;
                }
                return Data + entityCompOffset;
            }

            constexpr const void* GetComponent(const Handle& handle, const FName& componentId) const
            {
                if (!IsValid(handle))
                {
                    return nullptr;
                }
                uint32 entityCompOffset = GetOffsetToEntityComponent(handle.Id, componentId);
                if (entityCompOffset == Index<uint32>::None)
                {
                    return nullptr;
                }
                return Data + entityCompOffset;
            }

            template <class T>
            constexpr T* GetComponent(const Handle& handle)
            {
                void* dataPtr = GetComponent(handle, T::StaticTypeName);
                return static_cast<T*>(dataPtr);
            }

            template <class T>
            constexpr const T* GetComponent(const Handle& handle) const
            {
                const void* dataPtr = GetComponent(handle, T::StaticTypeName);
                return static_cast<const T*>(dataPtr);
            }

            // Gets the total size of an entity in the Data buffer.
            constexpr uint32 GetEntityTotalSize() const
            {
                return sizeof(ArchetypeInstance) + Definition.GetTotalSize();
            }

            // Gets the local offset of a component within an entity in the list.
            // Returns -1 if a component with the given id could not be found.
            constexpr uint32 GetComponentLocalOffset(const FName& componentId) const
            {
                uint32 index = Definition.IndexOfComponent(componentId);
                if (!Definition.IsValidIndex(index))
                {
                    return Index<uint32>::None;
                }
                return Definition[index].Offset;
            }

            void ForEachInstance(TFunction<void(const Handle&)>&& func) const
            {
                for (uint32 i = 0; i < NumInstances; ++i)
                {
                    const ArchetypeInstance* instance = GetEntityPtr(i);

                    if (instance->EntityId == EntityId::Invalid)
                    {
                        continue;
                    }

                    func(Handle(Id, i, instance->EntityId));
                }
            }

            template <class ...TComponents>
            void ForEachEntity(TFunction<void(EntityId, TComponents...)>&& func)
            {
                for (uint32 i = 0; i < NumInstances; ++i)
                {
                    ArchetypeInstance* instance = GetEntityPtr(i);

                    if (instance->EntityId == EntityId::Invalid)
                    {
                        continue;
                    }

                    func(instance->EntityId, GetComponentRef<TComponents>(i)...);
                }
            }

            template <class TCallback>
            void ForEachComponent(const Handle& handle, TCallback&& func)
            {
                for (uint8 i = 0; i < (uint8)Definition.GetNumComponents(); ++i)
                {
                    const ComponentDefinition& componentDefinition = Definition[i];
                    if (void* compPtr = GetComponent(handle, componentDefinition.Id))
                    {
                        func(componentDefinition, compPtr);
                    }
                }
            }

            template <class TCallback>
            void ForEachComponent(const Handle& handle, TCallback&& func) const
            {
                for (uint8 i = 0; i < Definition.GetNumComponents(); ++i)
                {
                    const ComponentDefinition& componentDefinition = Definition[i];
                    if (const void* compPtr = GetComponent(handle, componentDefinition.Id))
                    {
                        func(componentDefinition, compPtr);
                    }
                }
            }

            Handle GetFirstActiveEntity() const
            {
                return GetNextActiveEntity({ Id, 0, 0 });
            }

            Handle GetNextActiveEntity(const Handle& handle) const
            {
                if (!OwnsHandle(handle))
                {
                    return Handle();
                }

                Handle result;
                for (uint32 i = handle.Id; i < NumInstances; ++i)
                {
                    ArchetypeInstance* instance = GetEntityPtr(i);

                    if (instance->EntityId == EntityId::Invalid)
                    {
                        continue;
                    }

                    result = { Id, i, instance->EntityId };
                    break;
                }

                return result;
            }

            template <class ...TComponents>
            struct EntityComponentIter
            {
                EntityComponentIter(TArchetypeList* list, const Handle& handle)
                    : Curr(handle)
                    , List(list)
                {
                    if (!List->IsValid(Curr))
                        Curr = List->GetNextActiveEntity(Curr);
                }

                TTuple<EntityId, TComponents...> operator*() const
                {
                    return std::make_tuple(Curr.EntityId, ComponentAccessor<TComponents>::template GetComponentRef(*this, Curr.Id)...);
                }

                EntityComponentIter& operator++()
                {
                    Curr = List->GetNextActiveEntity(Curr);
                    return *this;
                }

                bool operator==(const EntityComponentIter& other) const
                {
                    return List == other.List && Curr == other.Curr;
                }

                Handle Curr;
                TArchetypeList* List;
            };

            template <class ...TComponents>
            EntityComponentIter<TComponents...> begin()
            {
                return EntityComponentIter<TComponents...>(this, GetFirstActiveEntity());
            }

            template <class ...TComponents>
            EntityComponentIter<TComponents...> end()
            {
                return EntityComponentIter<TComponents...>(this, Handle());
            }

        private:

            // Gets the total offset to the entity at the given index in the raw Data buffer.
            constexpr uint32 GetOffsetToEntity(uint32 index) const
            {
                return uint32(index * GetEntityTotalSize());
            }

            // Gets the total offset to the head of the components of an entity in the Data buffer.
            constexpr uint32 GetOffsetToEntityComponentHead(uint32 index) const
            {
                return GetOffsetToEntity(index) + sizeof(ArchetypeInstance);
            }

            // Gets the total offset to the component of a given entity.
            constexpr uint32 GetOffsetToEntityComponent(uint32 index, const FName& componentId) const
            {
                uint32 componentOffset = GetComponentLocalOffset(componentId);
                if (componentOffset == Index<uint32>::None)
                {
                    return Index<uint32>::None;
                }
                return GetOffsetToEntityComponentHead(index) + componentOffset;
            }

            // Gets a pointer to the Entity object at a given index.
            // Returns nullptr if there is no entity at that index.
            constexpr ArchetypeInstance* GetEntityPtr(uint32 index)
            {
                uint32 offset = GetOffsetToEntity(index);
                if (offset == Index<uint32>::None)
                {
                    return nullptr;
                }
                return reinterpret_cast<ArchetypeInstance*>(Data + offset);
            }

            // Gets a pointer to the Entity object at a given index.
            // Returns nullptr if there is no entity at that index.
            constexpr const ArchetypeInstance* GetEntityPtr(uint32 index) const
            {
                uint32 offset = GetOffsetToEntity(index);
                if (offset == Index<uint32>::None)
                {
                    return nullptr;
                }
                return reinterpret_cast<const ArchetypeInstance*>(Data + offset);
            }

            // Gets the address of the first component of an entity at a given index.
            constexpr void* GetEntityComponentHeadPtr(uint32 index)
            {
                uint32 offset = GetOffsetToEntityComponentHead(index);
                if (offset == Index<uint32>::None)
                {
                    return nullptr;
                }
                return Data + offset;
            }

            // Gets the address of the first component of an entity at a given index.
            constexpr const void* GetEntityComponentHeadPtr(uint32 index) const
            {
                uint32 offset = GetOffsetToEntityComponentHead(index);
                if (offset == Index<uint32>::None)
                {
                    return nullptr;
                }
                return Data + offset;
            }

            // Gets the address of a component of an entity at a given index.
            // Returns nullptr if the entity or component is not valid.
            constexpr void* GetEntityComponentPtr(uint32 index, const FName& componentId)
            {
                uint32 offset = GetOffsetToEntityComponent(index, componentId);
                if (offset == Index<uint32>::None)
                {
                    return nullptr;
                }
                return Data + offset;
            }

            // Gets the address of a component of an entity at a given index.
            // Returns nullptr if the entity or component is not valid.
            constexpr const void* GetEntityComponentPtr(uint32 index, const FName& componentId) const
            {
                uint32 offset = GetOffsetToEntityComponent(index, componentId);
                if (offset == Index<uint32>::None)
                {
                    return nullptr;
                }
                return Data + offset;
            }

            // Gets the address of a component of an entity at a given index.
            // Returns nullptr if the entity or component is not valid.
            template <class T>
            constexpr T* GetEntityComponentPtr(uint32 index)
            {
                void* dataPtr = GetEntityComponentPtr(index, T::StaticTypeName);
                return static_cast<T*>(dataPtr);
            }

            // Gets the address of a component of an entity at a given index.
            // Returns nullptr if the entity or component is not valid.
            template <class T>
            constexpr const T* GetEntityComponentPtr(uint32 index) const
            {
                const void* dataPtr = GetEntityComponentPtr(index, T::StaticTypeName);
                return static_cast<const T*>(dataPtr);
            }

            template <class T>
            constexpr T GetComponentRef(uint32 index)
            {
                auto ptr = GetEntityComponentPtr<Underlying_T<T>>(index);
                return ComponentAccessor<T>::GetComponentRef(ptr);
            }

            template <class T>
            constexpr T GetComponentRef(uint32 index) const
            {
                auto ptr = const_cast<Underlying_T<T>*>(GetEntityComponentPtr<Underlying_T<T>>(index));
                return ComponentAccessor<T>::GetComponentRef(ptr);
            }

            uint32 FindFreeSlot()
            {
                // Shortcut using free list
                if (FreeHead != Index<uint32>::None)
                {
                    if (ArchetypeInstance* freeHead = GetEntityPtr(FreeHead))
                    {
                        PHX_ASSERT(freeHead->EntityId == EntityId::Invalid);

                        uint32 slotIndex = FreeHead;
                        if (FreeHead == FreeTail)
                        {
                            FreeHead = FreeTail = Index<uint32>::None;
                        }
                        else
                        {
                            FreeHead = freeHead->NextFree;
                        }
                        
                        return slotIndex;
                    }                    
                }
                return Index<uint32>::None;
            }

            uint32 Id = 0;
            TArchetypeDefinition Definition;
            uint32 NumInstances = 0;
            uint32 NumActiveInstances = 0;
            uint32 FreeHead = Index<uint32>::None;
            uint32 FreeTail = Index<uint32>::None;
            uint8 Data[Capacity] = {};
        };

        template <class ...TComponents>
        struct EntityComponentSpan
        {
            template <class TArchetypeList>
            static EntityComponentSpan FromList(TArchetypeList& list, uint32 startingIndex)
            {
                EntityComponentSpan span;
                span.RawData = list.GetData();
                span.StartingIndex = startingIndex;
                span.InstanceCount = list.GetNumInstances();
                span.Step = list.GetEntityTotalSize();

                uint32 offsets[sizeof...(TComponents)] = { list.GetComponentLocalOffset(Underlying_T<TComponents>::StaticTypeName)... };
                memcpy(span.Offsets, offsets, sizeof...(TComponents) * sizeof(uint32));

                span.CheckRawData();

                return span;
            }

            uint32 GetStartIndex() const
            {
                CheckRawData();
                return StartingIndex;
            }

            uint32 GetInstanceCount() const
            {
                CheckRawData();
                return InstanceCount;
            }

            uint32 GetStep() const
            {
                CheckRawData();
                return Step;
            }

            uint32 GetGlobalIndex(uint32 localIndex) const
            {
                CheckRawData();
                return StartingIndex + localIndex;
            }

            TTuple<EntityId, uint32, TComponents...> operator[](uint32 index) const
            {
                CheckRawData();
                uint32 entityOffset = index * Step;
                uint8* dataPtr = static_cast<uint8*>(RawData) + entityOffset;
                const ArchetypeInstance* instance = reinterpret_cast<const ArchetypeInstance*>(dataPtr);
                dataPtr += sizeof(ArchetypeInstance);
                return MakeTuple(instance->EntityId, index, dataPtr, std::make_index_sequence<sizeof...(TComponents)>{});
            }

            struct ConstIter
            {
                ConstIter(const EntityComponentSpan* span, uint32 index)
                    : Span(span)
                {
                    Span->CheckRawData();
                    Index = Span->FindNextActiveEntity(index);
                }

                TTuple<EntityId, uint32, TComponents...> operator*() const
                {
                    Span->CheckRawData();
                    return Span->operator[](Index);
                }

                ConstIter& operator++()
                {
                    Span->CheckRawData();
                    Index = Span->FindNextActiveEntity(Index + 1);
                    return *this;
                }

                bool operator==(const ConstIter& other) const
                {
                    Span->CheckRawData();
                    return Span == other.Span && Index == other.Index;
                }

                uint32 Index;
                const EntityComponentSpan* Span;
            };

            void CheckRawData() const
            {
                uint64 d = reinterpret_cast<uint64>(RawData);
                if ((d & 0x000000FF00000000) == d)
                {
                    __debugbreak();
                }
            }

            ConstIter begin() const
            {
                CheckRawData();
                return ConstIter(this, 0);
            }

            ConstIter end() const
            {
                CheckRawData();
                return ConstIter(this, InstanceCount);
            }

        private:

            template <uint8 I, class T>
            T GetComponentRef(void* data) const
            {
                CheckRawData();
                auto ptr = reinterpret_cast<Underlying_T<T>*>(static_cast<uint8*>(data) + Offsets[I]);
                return ComponentAccessor<T>::GetComponentRef(ptr);
            }

            template <std::size_t ...Is>
            std::tuple<EntityId, uint32, TComponents...> MakeTuple(EntityId entityId, uint32 index, void* data, std::index_sequence<Is...>) const
            {
                CheckRawData();
                return { entityId, index, GetComponentRef<Is, TComponents>(data)... };
            }

            uint32 FindNextActiveEntity(uint32 index) const
            {
                CheckRawData();
                while (index < InstanceCount)
                {
                    uint32 entityOffset = index * Step;
                    const uint8* dataPtr = static_cast<uint8*>(RawData) + entityOffset;
                    const ArchetypeInstance* instance = reinterpret_cast<const ArchetypeInstance*>(dataPtr);
                    if (instance->EntityId != EntityId::Invalid)
                    {
                        break;
                    }
                    ++index;
                }
                return index;
            }

        public:

            uint32 StartingIndex = 0;
            uint32 InstanceCount = 0;
            uint32 Step = 0;
            uint32 Offsets[sizeof...(TComponents)] = {};
            void* RawData = nullptr;
        };

        using ArchetypeList = TArchetypeList<>;
    }
}
