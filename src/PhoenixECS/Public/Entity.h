
#pragma once

#include "DLLExport.h"
#include "ArchetypeHandle.h"
#include "Name.h"

namespace Phoenix
{
    namespace ECS
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
