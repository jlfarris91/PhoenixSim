
#pragma once

#include "Features.h"
#include "FixedArray.h"
#include "FixedPoint.h"
#include "Worlds.h"

#ifndef ECS_MAX_ENTITIES
#define ECS_MAX_ENTITIES MAXINT16
#endif

#ifndef ECS_MAX_COMPONENTS
#define ECS_MAX_COMPONENTS MAXINT16
#endif

#ifndef ECS_MAX_COMPONENT_SIZE
#define ECS_MAX_COMPONENT_SIZE (32 * sizeof(uint32))
#endif

namespace Phoenix
{
    namespace ECS
    {
        typedef uint32 entityid_t;
        
        struct PHOENIXSIM_API EntityId
        {
            static const EntityId Invalid;

            constexpr EntityId() : Id(0) {}
            constexpr EntityId(entityid_t raw) : Id(raw) {}

            operator entityid_t() const;
            EntityId& operator=(const entityid_t& id);

        private:
            entityid_t Id;
        };

        struct PHOENIXSIM_API Entity
        {
            EntityId Id = EntityId::Invalid;
            FName Kind;
            int32 ComponentTail = INDEX_NONE;
        };

        struct PHOENIXSIM_API Component
        {
            EntityId Owner = EntityId::Invalid;

            // The name of the component type.
            FName TypeName;

            // The index of the next component owned by a given entity.
            int32 Next = INDEX_NONE;

            // The raw data of the component.
            uint8 Data[ECS_MAX_COMPONENT_SIZE];
        };
        
        class PHOENIXSIM_API ISystem
        {
        public:
            virtual ~ISystem() = default;

            virtual void OnPreUpdate(WorldRef world) {}
            virtual void OnUpdate(WorldRef world) {}
            virtual void OnPostUpdate(WorldRef world) {}

            virtual void OnPreHandleAction(WorldRef world) {}
            virtual void OnHandleAction(WorldRef world) {}
            virtual void OnPostHandleAction(WorldRef world) {}
        };

        struct PHOENIXSIM_API ComponentType
        {
            FName TypeName;
            uint32 Head = INDEX_NONE;
        };

        struct PHOENIXSIM_API FeatureECSDynamicBlock
        {
            static const FName StaticName;

            TFixedArray<Entity, ECS_MAX_ENTITIES> Entities;
            TFixedArray<Component, ECS_MAX_COMPONENTS> Components;
            TFixedArray<EntityId, ECS_MAX_ENTITIES> Groups;
        };

        struct PHOENIXSIM_API FeatureECSCtorArgs
        {
            TArray<TSharedPtr<ISystem>> Systems;
        };

        class PHOENIXSIM_API FeatureECS : public IFeature
        {
        public:

            static const FName StaticName;

            FeatureECS();
            FeatureECS(const FeatureECSCtorArgs& args);

            FName GetName() const override;
            
            FeatureDefinition GetFeatureDefinition() override;
            
            void OnPreUpdate(WorldRef world, const FeatureUpdateArgs& args) override;
            void OnUpdate(WorldRef world, const FeatureUpdateArgs& args) override;
            void OnPostUpdate(WorldRef world, const FeatureUpdateArgs& args) override;
            
            void OnHandleAction(WorldRef world, const FeatureActionArgs& action) override;

            static bool IsEntityValid(WorldConstRef world, EntityId entityId);

            static int32 GetEntityIndex(EntityId entityId);
            
            static Entity* GetEntityPtr(WorldRef world, EntityId entityId);
            static const Entity* GetEntityPtr(WorldConstRef world, EntityId entityId);

            static Entity& GetEntityRef(WorldRef world, EntityId entityId);
            static const Entity& GetEntityRef(WorldConstRef world, EntityId entityId);

            static EntityId AcquireEntity(WorldRef world, FName kind);
            static bool ReleaseEntity(WorldRef world, EntityId entityId);

            static int32 GetIndexOfComponent(WorldConstRef world, EntityId entityId, FName componentType);

            static Component* GetComponentPtr(WorldRef world, EntityId entityId, FName componentType);
            static const Component* GetComponentPtr(WorldConstRef world, EntityId entityId, FName componentType);

            static Component& GetComponentRef(WorldRef world, EntityId entityId, FName componentType);
            static const Component& GetComponentRef(WorldConstRef world, EntityId entityId, FName componentType);

            template <class T>
            static T* GetComponentDataPtr(WorldRef world, EntityId entityId)
            {
                Component* comp = GetComponentPtr(world, entityId, T::StaticTypeName);
                if (!comp) return nullptr;
                return reinterpret_cast<T*>(&comp->Data[0]);
            }

            template <class T>
            static const T* GetComponentDataPtr(WorldConstRef world, EntityId entityId)
            {
                Component* comp = GetComponentPtr(world, entityId, T::StaticTypeName);
                if (!comp) return nullptr;
                return reinterpret_cast<const T*>(&comp->Data[0]);
            }

            template <class T>
            static T* AddComponent(WorldRef world, EntityId entityId, const T& defaultValue = {})
            {
                static_assert(sizeof(T) < ECS_MAX_COMPONENT_SIZE);
                Component* comp = AddComponent(world, entityId, T::StaticTypeName);
                if (!comp) return nullptr;
                *reinterpret_cast<T*>(&comp->Data[0]) = defaultValue;
                return reinterpret_cast<T*>(&comp->Data[0]);
            }

            static bool RemoveComponent(WorldRef world, EntityId entityId, FName componentType);

            static uint32 RemoveAllComponents(WorldRef world, EntityId entityId);

            template <class T>
            static void ForEachComponent(WorldRef world, EntityId entityId, const T& callback)
            {
                FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

                const Entity* entity = GetEntityPtr(world, entityId);
                if (!entity)
                {
                    return;
                }

                uint32 compIndex = GetEntityIndex(entityId);

                while (compIndex != INDEX_NONE)
                {
                    Component& comp = block.Components[compIndex];
                    if (comp.Owner != entityId)
                    {
                        return;
                    }

                    if (!callback(comp, compIndex))
                    {
                        return;
                    }

                    compIndex = comp.Next;
                }
            }

            template <class T>
            static void ForEachComponent(WorldConstRef world, EntityId entityId, const T& callback)
            {
                const FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

                const Entity* entity = GetEntityPtr(world, entityId);
                if (!entity)
                {
                    return;
                }

                uint32 compIndex = GetEntityIndex(entityId);

                while (compIndex != INDEX_NONE)
                {
                    const Component& comp = block.Components[compIndex];
                    if (comp.Owner != entityId)
                    {
                        return;
                    }

                    if (!callback(comp, compIndex))
                    {
                        return;
                    }

                    compIndex = comp.Next;
                }
            }

            void RegisterSystem(const TSharedPtr<ISystem>& system);
            
        private:

            static Component* AddComponent(WorldRef world, EntityId entityId, FName componentType);

            static void CompactWorldBuffer(WorldRef world);

            TArray<TSharedPtr<ISystem>> Systems;
            FeatureDefinition FeatureDefinition;
        };

        template <size_t I = 0, typename... Ts>
        std::enable_if_t<I == sizeof...(Ts), bool>
        contains_nullptr(const std::tuple<Ts...>& t)
        {
            // Base case: end of tuple, no nullptr found
            return false;
        }

        template <size_t I = 0, typename... Ts>
        std::enable_if_t<(I < sizeof...(Ts)), bool>
        contains_nullptr(const std::tuple<Ts...>& t)
        {
            using ElementType = std::tuple_element_t<I, std::tuple<Ts...>>;
            if constexpr (std::is_pointer_v<ElementType>)
            {
                if (std::get<I>(t) == nullptr)
                {
                    return true;
                }
            }
            // Recurse to the next element
            return contains_nullptr<I + 1>(t);
        }

        template <class ...TComponents>
        class EntityComponentsContainer
        {
        public:
            using ElementType = std::tuple<Entity*, std::add_pointer_t<TComponents>...>;

            EntityComponentsContainer() = default;

            EntityComponentsContainer(WorldRef world)
            {
                Refresh(world);
            }

            void Refresh(WorldRef world)
            {
                FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

                Components.Reset();

                for (Entity& entity : block.Entities)
                {
                    if (entity.Id == EntityId::Invalid)
                        continue;
            
                    auto components = std::make_tuple(&entity, FeatureECS::GetComponentDataPtr<TComponents...>(world, entity.Id));
                    if (contains_nullptr(components))
                        continue;

                    Components.PushBack(components);
                }
            }

            auto begin() { return Components.begin(); }
            auto end() { return Components.end(); }

        private:
            TFixedArray<ElementType, ECS_MAX_ENTITIES> Components;
        };
    }
}