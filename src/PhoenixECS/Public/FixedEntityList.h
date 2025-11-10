
#pragma once

#include "Entity.h"
#include "Containers/FixedArray.h"

namespace Phoenix
{
    namespace ECS
    {
        template <size_t N>
        class FixedEntityList
        {
        public:

            static constexpr size_t Capacity = N;

            constexpr size_t GetSize() const
            {
                return Entities.Num();
            }

            constexpr size_t GetNumActive() const
            {
                return NumActiveEntities;
            }
            
            constexpr bool IsValid(EntityId entityId) const
            {
                return GetEntityPtr(entityId) != nullptr;
            }

            static constexpr int32 GetEntityIndex(EntityId entityId)
            {
                return entityId % Capacity;
            }
            
            constexpr Entity* GetEntityPtr(EntityId entityId)
            {
                uint32 index = GetEntityIndex(entityId);
                if (!Entities.IsValidIndex(index))
                    return nullptr;
                Entity& entity = Entities[index];
                return entity.GetId() == entityId ? &entity : nullptr;                
            }

            constexpr const Entity* GetEntityPtr(EntityId entityId) const
            {
                uint32 index = GetEntityIndex(entityId);
                if (!Entities.IsValidIndex(index))
                    return nullptr;
                const Entity& entity = Entities[index];
                return entity.GetId() == entityId ? &entity : nullptr;
            }

            constexpr Entity& GetEntityRef(EntityId entityId)
            {
                uint32 index = GetEntityIndex(entityId);
                Entity& entity = Entities[index];
                return entity.GetId() == entityId ? entity : Entities[0];
            }

            constexpr const Entity& GetEntityRef(EntityId entityId) const
            {
                uint32 index = GetEntityIndex(entityId);
                const Entity& entity = Entities[index];
                return entity.GetId() == entityId ? entity : Entities[0];
            }

            constexpr EntityId Acquire(const FName& kind)
            {
                // Find the first invalid entity index
                uint32 entityIdx = 1;
                for (; entityIdx < Capacity; ++entityIdx)
                {
                    if (Entities[entityIdx].GetId() == EntityId::Invalid)
                    {
                        break;
                    }
                }

                if (entityIdx == Capacity)
                {
                    return EntityId::Invalid;
                }

                if (!Entities.IsValidIndex(entityIdx))
                {
                    Entities.SetNum(entityIdx + 1);
                }

                Entity& entity = Entities[entityIdx];
                entity.Kind = kind;
                entity.Handle = ArchetypeHandle(entityIdx);
                entity.TagHead = INDEX_NONE;

                ++NumActiveEntities;

                return entityIdx;
            }

            constexpr bool Release(EntityId entityId)
            {
                if (!IsValid(entityId))
                {
                    return false;
                }

                int32 index = GetEntityIndex(entityId);
                Entity& entity = Entities[index];
                entity.Kind = FName::None;
                entity.Handle = ArchetypeHandle();

                --NumActiveEntities;

                return true;
            }

        private:

            TFixedArray<Entity, Capacity> Entities;
            size_t NumActiveEntities = 0;
        };
    }
}
