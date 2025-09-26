
#pragma once

#include "Features.h"
#include "FixedArray.h"
#include "Worlds.h"
#include "FixedTransform.h"

#ifndef ECS_MAX_ENTITIES
#define ECS_MAX_ENTITIES MAXINT16
#endif

#ifndef ECS_MAX_COMPONENTS
#define ECS_MAX_COMPONENTS MAXINT16 << 1
#endif

#ifndef ECS_MAX_COMPONENT_SIZE
#define ECS_MAX_COMPONENT_SIZE (64 - (sizeof(hash_t) + sizeof(int32))) + 64
#endif

namespace Phoenix
{
    namespace ECS
    {
        struct FeatureECSDynamicBlock;
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
            int32 ComponentHead = INDEX_NONE;
        };

        struct PHOENIXSIM_API Component
        {
            // The name of the component type.
            FName TypeName;

            // The index of the next component owned by a given entity.
            int32 Next = INDEX_NONE;

            // The raw data of the component.
            uint8 Data[ECS_MAX_COMPONENT_SIZE];
        };

        struct PHOENIXSIM_API TransformComponent
        {
            static constexpr FName StaticName = "Transform"_n;

            // The id of another entity that the owning entity is attached to.
            // Note that this cannot be the entity that owns the body component.
            EntityId AttachParent = EntityId::Invalid;

            // The relative transform of the entity.
            // Relative to the origin if not attached to another entity.
            Transform2D Transform;

            // Morton z-code used for spacial sorting of entities.
            uint64 ZCode = 0;
        };

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

            void Refresh(WorldRef world);

            auto begin() { return Components.begin(); }
            auto end() { return Components.end(); }

        private:
            TFixedArray<ElementType, ECS_MAX_ENTITIES> Components;
        };

        struct PHOENIXSIM_API SystemUpdateArgs
        {
            simtime_t SimTime = 0;
            DeltaTime DeltaTime;
        };

        struct PHOENIXSIM_API SystemActionArgs
        {
            simtime_t SimTime = 0;
            Action Action;
        };

        class PHOENIXSIM_API ISystem
        {
        public:
            virtual ~ISystem() = default;

            virtual FName GetName() { return FName::None; }

            virtual void OnPreUpdate(WorldRef world, const SystemUpdateArgs& args) {}
            virtual void OnUpdate(WorldRef world, const SystemUpdateArgs& args) {}
            virtual void OnPostUpdate(WorldRef world, const SystemUpdateArgs& args) {}

            virtual void OnPreHandleAction(WorldRef world, const SystemActionArgs& args) {}
            virtual void OnHandleAction(WorldRef world, const SystemActionArgs& args) {}
            virtual void OnPostHandleAction(WorldRef world, const SystemActionArgs& args) {}
        };

        struct PHOENIXSIM_API FeatureECSDynamicBlock
        {
            static constexpr FName StaticName = "FeatureECSDynamicBlock"_n;

            TFixedArray<Entity, ECS_MAX_ENTITIES> Entities;
            TFixedArray<Component, ECS_MAX_COMPONENTS> Components;
            TFixedArray<EntityId, ECS_MAX_ENTITIES> Groups;
        };

        struct PHOENIXSIM_API EntityTransform
        {
            EntityId EntityId;
            TransformComponent* TransformComponent;
            uint64 ZCode = 0;
        };

        struct PHOENIXSIM_API MovementComponent
        {
            static constexpr FName StaticName = "MoveToCenterComponent"_n;
            Speed Speed = 0;
        };

        struct PHOENIXSIM_API FeatureECSScratchBlock
        {
            static constexpr FName StaticName = "FeatureECSScratchBlock"_n;

            EntityComponentsContainer<TransformComponent> EntityTransforms;
            TFixedArray<EntityTransform, ECS_MAX_ENTITIES> SortedEntities;
        };

        struct PHOENIXSIM_API FeatureECSCtorArgs
        {
            TArray<TSharedPtr<ISystem>> Systems;
        };

        class PHOENIXSIM_API FeatureECS : public IFeature
        {
        public:

            static constexpr FName StaticName = "FeatureECS"_n;

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
                Component* comp = GetComponentPtr(world, entityId, T::StaticName);
                if (!comp) return nullptr;
                return reinterpret_cast<T*>(&comp->Data[0]);
            }

            template <class T>
            static const T* GetComponentDataPtr(WorldConstRef world, EntityId entityId)
            {
                Component* comp = GetComponentPtr(world, entityId, T::StaticName);
                if (!comp) return nullptr;
                return reinterpret_cast<const T*>(&comp->Data[0]);
            }

            template <class T>
            static T* AddComponent(WorldRef world, EntityId entityId, const T& defaultValue = {})
            {
                static_assert(sizeof(T) < ECS_MAX_COMPONENT_SIZE);
                Component* comp = AddComponent(world, entityId, T::StaticName);
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

                int32 compIndex = entity->ComponentHead;

                while (compIndex != INDEX_NONE)
                {
                    Component& comp = block.Components[compIndex];
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

                int32 compIndex = entity->ComponentHead;

                while (compIndex != INDEX_NONE)
                {
                    const Component& comp = block.Components[compIndex];
                    if (!callback(comp, compIndex))
                    {
                        return;
                    }
                    compIndex = comp.Next;
                }
            }

            void RegisterSystem(const TSharedPtr<ISystem>& system);

            static void GetWorldTransform(WorldConstRef world, EntityId entityId, Transform2D& outTransform);

            static void QueryEntitiesInRange(WorldConstRef& world, const Vec2& pos, Distance range, TArray<EntityTransform>& outEntities);

        private:

            static Component* AddComponent(WorldRef world, EntityId entityId, FName componentType);

            static void SortEntitiesByZCode(WorldRef world);

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
        
        template <class ... TComponents>
        void EntityComponentsContainer<TComponents...>::Refresh(WorldRef world)
        {
            FeatureECSDynamicBlock& block = world.GetBlockRef<FeatureECSDynamicBlock>();

            Components.Reset();

            for (Entity& entity : block.Entities)
            {
                if (entity.Id == EntityId::Invalid)
                    continue;
            
                auto components = std::make_tuple(&entity, FeatureECS::GetComponentDataPtr<TComponents>(world, entity.Id)...);
                if (contains_nullptr(components))
                    continue;

                Components.PushBack(components);
            }
        }
    }
}