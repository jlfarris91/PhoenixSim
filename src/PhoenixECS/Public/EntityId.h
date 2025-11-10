
#pragma once

#include "Platform.h"
#include "DLLExport.h"

namespace Phoenix
{
    namespace ECS2
    {
        typedef uint32 entityid_t;
        
        struct PHOENIXECS_API EntityId
        {
            static const EntityId Invalid;

            constexpr EntityId() : Id(0) {}
            constexpr EntityId(entityid_t raw) : Id(raw) {}

            operator entityid_t() const;
            EntityId& operator=(const entityid_t& id);

        private:
            entityid_t Id;
        };
    }
}
