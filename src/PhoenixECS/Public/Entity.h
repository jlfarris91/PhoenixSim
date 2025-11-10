
#pragma once

#include "ArchetypeHandle.h"
#include "DLLExport.h"
#include "Name.h"

namespace Phoenix
{
    namespace ECS2
    {
        struct PHOENIXECS_API Entity
        {
            ArchetypeHandle Handle;
            FName Kind;
            int32 TagHead = INDEX_NONE;

            constexpr EntityId GetId() const
            {
                return Handle.GetEntityId();
            }
        };
    }
}
