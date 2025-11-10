
#pragma once

#include "DLLExport.h"
#include "EntityId.h"

namespace Phoenix
{
    namespace ECS2
    {
        template <class TArchetypeDefinition, uint32 N>
        class TArchetypeList;

        struct PHOENIXECS_API ArchetypeHandle
        {
            template <class TArchetypeDefinition, uint32 N>
            friend class TArchetypeList;

            ArchetypeHandle() = default;

            uint32 GetOwnerId() const
            {
                return OwnerId;
            }

            constexpr EntityId GetEntityId() const
            {
                return EntityId;
            }

            bool operator==(const ArchetypeHandle& other) const = default;

        private:

            ArchetypeHandle(uint32 ownerId, uint32 id, EntityId entityId)
                : OwnerId(ownerId)
                , Id(id)
                , EntityId(entityId)
            {
            }

            uint32 OwnerId = Index<uint32>::None;
            uint32 Id = Index<uint32>::None;
            EntityId EntityId = EntityId::Invalid;
        };
    }
}