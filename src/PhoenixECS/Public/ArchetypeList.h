
#pragma once

#include "ArchetypeHandle.h"
#include "EntityId.h"
#include "Name.h"
#include "Platform.h"

namespace Phoenix
{
    namespace ECS2
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

        // Archetype data is tightly packed into the Data buffer.
        // Data: [Entity0][Comp0][Comp1][Entity1][Comp0][Comp1]...
        template <class TArchetypeDefinition, uint32 N>
        class TArchetypeList
        {
        public:

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

            constexpr uint8* GetData()
            {
                return Data;
            }

            constexpr uint8* GetData() const
            {
                return Data;
            }

            constexpr uint32 GetHighWaterMark() const
            {
                return HighWaterMark;
            }

            constexpr bool IsFull() const
            {
                return Size + Definition.TotalSize >= N;
            }

            constexpr bool IsValid(const Handle& handle) const
            {
                if (!OwnsHandle(handle) || handle.Id >= HighWaterMark)
                {
                    return false;
                }

                const Entity* instance = GetEntityPtr(handle.Id);
                return instance && instance->EntityId == handle.EntityId;
            }

            constexpr bool OwnsHandle(const Handle& handle) const
            {
                return handle.OwnerId == Id;
            }

            Handle Allocate(EntityId entityId)
            {
                if (IsFull())
                {
                    return Handle();
                }

                Handle handle = { Id, HighWaterMark++, entityId };
                Entity* instance = GetEntityPtr(handle.Id);
                instance->EntityId = entityId;

                void* entityComponentDataPtr = GetEntityComponentHeadPtr(handle.Id);
                PHX_ASSERT(entityComponentDataPtr);

                Definition.Construct(entityComponentDataPtr);

                ++NumActiveEntities;

                return handle;
            }

            bool Deallocate(const Handle& handle)
            {
                if (!OwnsHandle(handle))
                {
                    return false;
                }

                Entity* instance = GetEntityPtr(handle.Id);
                if (!instance || instance->EntityId != handle.EntityId)
                {
                    return false;
                }

                instance->EntityId = EntityId::Invalid;

                void* entityComponentDataPtr = GetEntityComponentHeadPtr(handle.Id);
                PHX_ASSERT(entityComponentDataPtr);

                Definition.Deconstruct(entityComponentDataPtr);

                --NumActiveEntities;

                return true;
            }

            constexpr void* GetComponentPtr(const Handle& handle, const FName& componentId)
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

            constexpr const void* GetComponentPtr(const Handle& handle, const FName& componentId) const
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
            constexpr T* GetComponentPtr(const Handle& handle)
            {
                void* dataPtr = GetComponentPtr(handle, T::StaticTypeName);
                return static_cast<T*>(dataPtr);
            }

            template <class T>
            constexpr const T* GetComponentPtr(const Handle& handle) const
            {
                const void* dataPtr = GetComponentPtr(handle, T::StaticTypeName);
                return static_cast<const T*>(dataPtr);
            }

            // Gets the total size of an entity in the Data buffer.
            constexpr uint32 GetEntityTotalSize() const
            {
                return sizeof(Entity) + Definition.GetTotalSize();
            }

            // Gets the local offset of a component within an entity in the list.
            // Returns -1 if a component with the given id could not be found.
            constexpr uint32 GetComponentLocalOffset(const FName& componentId) const
            {
                uint32 index = Definition.IndexOfComponent(componentId);
                if (index == Index<uint32>::None)
                {
                    return Index<uint32>::None;
                }
                return Definition[index].Offset;
            }

            template <class ...TComponents>
            void ForEachEntity(TFunction<void(EntityId, TComponents...)>&& func)
            {
                for (uint32 i = 0; i < HighWaterMark; ++i)
                {
                    Entity* instance = GetEntityPtr(i);

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
                for (uint8 i = 0; i < Definition.GetNumComponents(); ++i)
                {
                    ComponentDefinition& componentDefinition = Definition[i];
                    if (void* compPtr = GetComponentPtr(handle, componentDefinition.Id))
                    {
                        func(compPtr);
                    }
                }
            }

            template <class TCallback>
            void ForEachComponent(const Handle& handle, TCallback&& func) const
            {
                for (uint8 i = 0; i < Definition.GetNumComponents(); ++i)
                {
                    ComponentDefinition& componentDefinition = Definition[i];
                    if (const void* compPtr = GetComponentPtr(handle, componentDefinition.Id))
                    {
                        func(compPtr);
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
                for (uint32 i = handle.Id; i < HighWaterMark; ++i)
                {
                    Entity* instance = GetEntityPtr(i);

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

            struct Entity
            {
                EntityId EntityId;
            };

            // Gets the total offset to the entity at the given index in the raw Data buffer.
            constexpr uint32 GetOffsetToEntity(uint32 index) const
            {
                return uint32(index * GetEntityTotalSize());
            }

            // Gets the total offset to the head of the components of an entity in the Data buffer.
            constexpr uint32 GetOffsetToEntityComponentHead(uint32 index) const
            {
                return GetOffsetToEntity(index) + sizeof(Entity);
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
            constexpr Entity* GetEntityPtr(uint32 index)
            {
                uint32 offset = GetOffsetToEntity(index);
                if (offset == Index<uint32>::None)
                {
                    return nullptr;
                }
                return reinterpret_cast<Entity*>(Data + offset);
            }

            // Gets a pointer to the Entity object at a given index.
            // Returns nullptr if there is no entity at that index.
            constexpr const Entity* GetEntityPtr(uint32 index) const
            {
                uint32 offset = GetOffsetToEntity(index);
                if (offset == Index<uint32>::None)
                {
                    return nullptr;
                }
                return reinterpret_cast<const Entity*>(Data + offset);
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

            uint32 Id = 0;
            TArchetypeDefinition Definition;
            uint32 Size = 0;
            uint32 NumActiveEntities = 0;
            uint32 HighWaterMark = 0;
            uint8 Data[N] = {};
        };

        template <class ...TComponents>
        struct EntityComponentSpan
        {
            template <class TArchetypeList>
            static EntityComponentSpan FromList(TArchetypeList& list)
            {
                EntityComponentSpan span;
                span.RawData = list.GetData();
                span.Num = list.GetHighWaterMark();
                span.Step = list.GetEntityTotalSize();

                uint32 offsets[sizeof...(TComponents)] = { list.GetComponentLocalOffset(Underlying_T<TComponents>::StaticTypeName)... };
                memcpy(span.Offsets, offsets, sizeof...(TComponents) * sizeof(uint32));

                return span;
            }

            TTuple<EntityId, TComponents...> operator[](uint32 index) const
            {
                uint32 entityOffset = index * Step;
                uint8* dataPtr = static_cast<uint8*>(RawData) + entityOffset;
                EntityId entityId = *reinterpret_cast<EntityId*>(dataPtr);
                dataPtr += sizeof(EntityId);
                return MakeTuple(entityId, dataPtr, std::make_index_sequence<sizeof...(TComponents)>{});
            }

            struct ConstIter
            {
                ConstIter(const EntityComponentSpan* span, uint32 index) : Index(index), Span(span) {}

                TTuple<EntityId, TComponents...> operator*() const
                {
                    return Span->operator[](Index);
                }

                ConstIter& operator++()
                {
                    ++Index;
                    return *this;
                }

                bool operator==(const ConstIter& other) const
                {
                    return Span == other.Span && Index == other.Index;
                }

                uint32 Index;
                const EntityComponentSpan* Span;
            };

            ConstIter begin() const
            {
                return ConstIter(this, 0);
            }

            ConstIter end() const
            {
                return ConstIter(this, Num);
            }

        private:

            template <uint8 I, class T>
            T GetComponentRef(void* data) const
            {
                auto ptr = reinterpret_cast<Underlying_T<T>*>(static_cast<uint8*>(data) + Offsets[I]);
                return ComponentAccessor<T>::GetComponentRef(ptr);
            }

            template <std::size_t ...Is>
            std::tuple<EntityId, TComponents...> MakeTuple(EntityId entityId, void* data, std::index_sequence<Is...>) const
            {
                return { entityId, GetComponentRef<Is, TComponents>(data)... };
            }

            uint32 Num = 0;
            uint32 Step = 0;
            uint32 Offsets[sizeof...(TComponents)] = {};
            void* RawData = nullptr;
        };
    }
}
