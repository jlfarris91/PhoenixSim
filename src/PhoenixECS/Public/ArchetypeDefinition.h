
#pragma once

#include "Platform.h"
#include "Name.h"
#include "Optional.h"
#include "Reflection.h"
#include "Containers/FixedArray.h"

namespace Phoenix
{
    namespace ECS
    {
        struct ComponentDefinition
        {
            FName Id = 0;
            uint16 Size = 0;
            uint16 Offset = 0;
            const TypeDescriptor* TypeDescriptor;

            template <class T>
            static ComponentDefinition Create(const FName& id = T::StaticTypeName)
            {
                const struct TypeDescriptor& descriptor = T::GetStaticTypeDescriptor();
                return { id, (uint16)descriptor.GetSize(), 0, &descriptor };
            }
        };

        template <uint8 MaxComponents>
        struct TArchetypeDefinition
        {
            TArchetypeDefinition() = default;

            TArchetypeDefinition(const ComponentDefinition* comps, uint8 n)
            {
                PHX_ASSERT(n <= MaxComponents);

                TotalSize = 0;
                Id = 0;
                for (uint8 i = 0; i < n; ++i)
                {
                    ComponentDefinition& def = Components.Add_GetRef(comps[i]);
                    def.Offset = TotalSize;

                    TotalSize += def.Size;
                    Id += def.Id;
                }
            }

            template <class ...TComponents>
            static TArchetypeDefinition Create(const TOptional<FName>& id)
            {
                static const ComponentDefinition comps[sizeof...(TComponents)] =
                {
                    ComponentDefinition::Create<TComponents>()...
                };
                TArchetypeDefinition definition(comps, sizeof...(TComponents));
                if (id.IsSet())
                {
                    definition.Id = id.Get();
                }
                return definition;
            }

            FName GetId() const
            {
                return Id;
            }

            uint16 GetNumComponents() const
            {
                return Components.Num();
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

            constexpr bool IsValidIndex(size_t index) const
            {
                return Components.IsValidIndex(index);
            }

            uint16 IndexOfComponent(const FName& componentId) const
            {
                for (uint32 i = 0; i < Components.Num(); ++i)
                {
                    if (Components[i].Id == componentId)
                        return i;
                }
                return -1;
            }

            void Construct(void* data) const
            {
                for (const ComponentDefinition& componentDefinition : Components)
                {
                    uint8* compDataPtr = static_cast<uint8*>(data) + componentDefinition.Offset;
                    if (const TypeDescriptor* descriptor = componentDefinition.TypeDescriptor)
                    {
                        descriptor->DefaultConstruct(compDataPtr);
                    }
                    else
                    {
                        memset(compDataPtr, 0, componentDefinition.Size);
                    }
                }
            }

            void Deconstruct(void* data) const
            {
                for (const ComponentDefinition& componentDefinition : Components)
                {
                    uint8* compDataPtr = static_cast<uint8*>(data) + componentDefinition.Offset;
                    if (const TypeDescriptor* descriptor = componentDefinition.TypeDescriptor)
                    {
                        descriptor->Destruct(compDataPtr);
                    }
                    else
                    {
                        memset(compDataPtr, 0, componentDefinition.Size);
                    }
                }
            }

            auto begin() { return Components.begin(); }
            auto end() { return Components.end(); }

            auto begin() const { return Components.begin(); }
            auto end() const { return Components.end(); }

            FName Id = 0;
            TFixedArray<ComponentDefinition, MaxComponents> Components;
            uint16 TotalSize = 0;
        };
    }
}
