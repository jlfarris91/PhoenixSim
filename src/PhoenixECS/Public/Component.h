
#pragma once

#include "Reflection.h"
#include "DLLExport.h"

namespace Phoenix
{
    namespace ECS
    {
        struct PHOENIXECS_API IComponent
        {
            PHX_DECLARE_INTERFACE(IComponent)
        };
    }
}

#define PHX_ECS_DECLARE_COMPONENT_BEGIN(name) PHX_DECLARE_DERIVED_TYPE_BEGIN(name, IComponent)
#define PHX_ECS_DECLARE_COMPONENT_END() PHX_DECLARE_DERIVED_TYPE_END()

#define PHX_ECS_DECLARE_COMPONENT(name) \
    PHX_ECS_DECLARE_COMPONENT_BEGIN(name) \
    PHX_ECS_DECLARE_COMPONENT_END()