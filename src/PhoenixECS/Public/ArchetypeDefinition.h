
#pragma once

#include <algorithm>

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

            TArchetypeDefinition(const FName& id, const ComponentDefinition* comps, uint8 n)
                : Id(id)
            {
                PHX_ASSERT(n <= MaxComponents);

                for (uint8 i = 0; i < n; ++i)
                {
                    Components.PushBack(comps[i]);
                }

                OnComponentsChanged();
            }

            template <class ...TComponents>
            static TArchetypeDefinition Create(const FName& id = FName::None)
            {
                static const ComponentDefinition comps[sizeof...(TComponents)] =
                {
                    ComponentDefinition::Create<TComponents>()...
                };
                return TArchetypeDefinition(id, comps, sizeof...(TComponents));
            }

            // Returns a new archetype definition with the newly added component.
            static bool AddComponent(
                const TArchetypeDefinition& baseArchDef,
                const ComponentDefinition& componentDefinition,
                TArchetypeDefinition& outArchDef)
            {
                for (size_t i = 0; i < baseArchDef.Components.Num(); ++i)
                {
                    if (baseArchDef.Components[i].Id == componentDefinition.Id)
                        return false;
                }
                
                outArchDef = baseArchDef;
                outArchDef.Id = FName::None;
                outArchDef.Components.PushBack(componentDefinition);
                outArchDef.OnComponentsChanged();
                return true;
            }

            // Returns a new archetype definition with the newly added component.
            template <class TComponent>
            static bool AddComponent(const TArchetypeDefinition& baseArchDef, TArchetypeDefinition& outArchDef)
            {
                ComponentDefinition compDef = ComponentDefinition::Create<TComponent>();
                return AddComponent(baseArchDef, compDef, outArchDef);
            }

            // Returns a new archetype definition without the given component.
            static bool RemoveComponent(
                const TArchetypeDefinition& baseArchDef,
                const FName& componentId,
                TArchetypeDefinition& outArchDef)
            {
                uint16 componentIndex = baseArchDef.IndexOfComponent(componentId);
                if (componentIndex == Index<uint16>::None)
                {
                    return false;
                }
                
                outArchDef = baseArchDef;
                outArchDef.Id = FName::None;
                outArchDef.Components.RemoveAt(componentIndex);
                outArchDef.OnComponentsChanged();
                return true;
            }

            // Returns a new archetype definition with the newly added component.
            template <class TComponent>
            static bool RemoveComponent(const TArchetypeDefinition& baseArchDef, TArchetypeDefinition& outArchDef)
            {
                return RemoveComponent(baseArchDef, TComponent::StaticTypeName, outArchDef);
            }

            constexpr FName GetId() const
            {
                return Id;
            }

            constexpr hash32_t GetArchetypeHash() const
            {
                return Hash;
            }

            constexpr bool HasIdOrHash(const FName& archetypeIdOrHash) const
            {
                return Id == archetypeIdOrHash || Hash == (hash32_t)archetypeIdOrHash;
            }

            constexpr uint8 GetNumComponents() const
            {
                return (uint8)Components.Num();
            }

            constexpr uint16 GetTotalSize() const
            {
                return TotalSize;
            }

            constexpr const ComponentDefinition& operator[](uint32 index) const
            {
                PHX_ASSERT(index < MaxComponents);
                return Components[index];
            }

            constexpr bool IsValidIndex(size_t index) const
            {
                return Components.IsValidIndex(index);
            }

            constexpr uint16 IndexOfComponent(const FName& componentId) const
            {
                for (size_t i = 0; i < Components.Num(); ++i)
                {
                    if (Components[i].Id == componentId)
                        return (uint16)i;
                }
                return Index<uint16>::None;
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

        private:

            void OnComponentsChanged()
            {
                // Sort components by id so that the archetype id is stable
                std::ranges::sort(
                    Components,
                    [](const ComponentDefinition& a, const ComponentDefinition& b)
                    {
                        return (hash32_t)a.Id < (hash32_t)b.Id;
                    });

                TotalSize = 0;
                Hash = 0;

                bool generateId = FName::IsNoneOrEmpty(Id);

                for (uint8 i = 0; i < (uint8)Components.Num(); ++i)
                {
                    Components[i].Offset = TotalSize;
                    TotalSize += Components[i].Size;
                    Hash = Hashing::FN1VA32Combine(Hash, (hash32_t)Components[i].Id);
                    if (generateId)
                    {
                        Id += Components[i].Id;
                    }
                }
            }

            FName Id;
            hash32_t Hash = 0;
            TFixedArray<ComponentDefinition, MaxComponents> Components;
            uint16 TotalSize = 0;
        };
    }
}
