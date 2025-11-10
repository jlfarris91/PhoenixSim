
#pragma once

#include "DLLExport.h"
#include "Name.h"

namespace Phoenix
{
    namespace ECS2
    {
        struct PHOENIXECS_API EntityTag
        {
            FName TagName;
            int32 Next = INDEX_NONE;
        };
    }
}