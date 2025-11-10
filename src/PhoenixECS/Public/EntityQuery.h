
#pragma once

#include <algorithm>
#include <functional>

#include "Platform.h"
#include "ArchetypeDefinition.h"
#include "EntityId.h"
#include "Name.h"
#include "Optional.h"
#include "Worlds.h"

namespace Phoenix
{
    namespace ECS2
    {
        enum class EComponentAccess : uint8
        {
            Read = 1,
            Write = 2,
            ReadWrite = 3
        };

        template <class>
        struct ComponentAccessFromT {};

        template <class T>
        struct ComponentAccessFromT<const T&> { static constexpr EComponentAccess ComponentAccess = EComponentAccess::Read; };

        template <class T>
        struct ComponentAccessFromT<T&> { static constexpr EComponentAccess ComponentAccess = EComponentAccess::ReadWrite; };

        template <class T, class TEquality = std::equal_to<T>>
        struct EntityQueryFilterSet
        {
            TArray<T> Items;

            EntityQueryFilterSet& AddAll(const EntityQueryFilterSet& other)
            {
                static TEquality equality;
                for (const T& otherItem : other.Items)
                {
                    if (std::find(Items.begin(), Items.end(), otherItem) == Items.end())
                    {
                        Items.push_back(otherItem);
                    }
                }
                return *this;
            }

            EntityQueryFilterSet& RemoveAll(const EntityQueryFilterSet& other)
            {
                static TEquality equality;
                erase_if(Items, [&](const T& item)
                {
                    return std::ranges::any_of(other.Items, [&](const T& otherItem)
                    {
                        return equality(item, otherItem);
                    });
                });
                return *this;
            }
        };

        using EntityQueryFilterComponentSet = EntityQueryFilterSet<TTuple<FName, EComponentAccess>>;
        using EntityQueryFilterTagSet = EntityQueryFilterSet<FName>;

        template <class TEntityManager>
        class EntityQueryBuilder;

        template <class ...TComponents>
        using TEntityQueryWorkFunc = TFunction<void(WorldRef world, EntityId, TComponents...)>;

        template <class ...TComponents>
        using TEntityQueryWorkBufferFunc = TFunction<void(WorldRef world, const EntityComponentSpan<TComponents...>&)>;

        struct EntityQuery
        {
            template <class TArchetypeDefinition = TArchetypeDefinition<8>>
            bool PassesFilter(const TArchetypeDefinition& definition) const
            {
                // None
                if (!ComponentsNone.Items.empty())
                {
                    for (auto && [id, access] : ComponentsNone.Items)
                    {
                        for (const ComponentDefinition& comp : definition)
                        {
                            if (comp.Id == id)
                                return false;
                        }
                    }
                }
                // Any
                if (!ComponentsAny.Items.empty())
                {
                    bool any = false;
                    for (auto && [id, access] : ComponentsAny.Items)
                    {
                        for (const ComponentDefinition& comp : definition)
                        {
                            if (comp.Id == id)
                            {
                                any = true;
                                break;
                            }
                        }
                        if (any)
                        {
                            break;
                        }
                    }
                    if (!any)
                    {
                        return false;
                    }                    
                }
                // All
                if (!ComponentsAll.Items.empty())
                {
                    bool all = true;
                    for (auto && [id, access] : ComponentsAll.Items)
                    {
                        bool found = false;
                        for (const ComponentDefinition& comp : definition)
                        {
                            if (comp.Id == id)
                            {
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            all = false;
                            break;
                        }
                    }
                    if (!all)
                    {
                        return false;
                    }
                }
                return true;
            }

            void Reset()
            {
                ArchetypeId.Reset();
                EntityName.Reset();
                ComponentsAll.Items.clear();
                ComponentsAny.Items.clear();
                ComponentsNone.Items.clear();
                TagsAll.Items.clear();
                TagsAny.Items.clear();
                TagsNone.Items.clear();
            }

        private:

            template <class T>
            friend class EntityQueryBuilder;

            TOptional<FName> ArchetypeId;
            TOptional<FName> EntityName;
            EntityQueryFilterComponentSet ComponentsAll;
            EntityQueryFilterComponentSet ComponentsAny;
            EntityQueryFilterComponentSet ComponentsNone;
            EntityQueryFilterTagSet TagsAll;
            EntityQueryFilterTagSet TagsAny;
            EntityQueryFilterTagSet TagsNone;
        };

        template <class TEntityManager>
        struct IEntityQueryWorker
        {
            virtual ~IEntityQueryWorker() = default;
            virtual void Execute(WorldRef world, TEntityManager& em) = 0;
        };

        template <class TEntityManager, class ...TComponents>
        struct TEntityQueryWorker : IEntityQueryWorker<TEntityManager>
        {
            using TArchetypeList = typename TEntityManager::TArchetypeList;

            TEntityQueryWorker(TArchetypeList* list, const TEntityQueryWorkFunc<TComponents...>& func) : List(list), Func(func)
            {
                PHX_ASSERT(List);
            }

            void Execute(WorldRef world, TEntityManager& em) override
            {
                List->template ForEachEntity<TComponents...>([&](EntityId entityId, TComponents ...components)
                {
                    Func(world, entityId, Forward<TComponents>(components)...);
                });
            }

            TArchetypeList* List;
            TEntityQueryWorkFunc<TComponents...> Func;
        };

        template <class TEntityManager, class ...TComponents>
        struct TEntityQueryBufferWorker : IEntityQueryWorker<TEntityManager>
        {
            using TArchetypeList = typename TEntityManager::TArchetypeList;

            TEntityQueryBufferWorker(TArchetypeList* list, const TEntityQueryWorkBufferFunc<TComponents...>& func) : List(list), Func(func)
            {
                PHX_ASSERT(List);
            }

            void Execute(WorldRef world, TEntityManager& em) override
            {
                Func(world, EntityComponentSpan<TComponents...>::FromList(*List));
            }

            TArchetypeList* List;
            TEntityQueryWorkBufferFunc<TComponents...> Func;
        };
        
        template <class TEntityManager>
        class EntityQueryBuilder
        {
        public:

            EntityQueryBuilder(TEntityManager* entityManager) : EntityManager(entityManager) {}

            EntityQueryBuilder& Reset()
            {
                Query.Reset();
                return *this;
            }

            EntityQueryBuilder& WithArchetype(const FName& name)
            {
                Query.ArchetypeId = name;
                return *this;
            }

            EntityQueryBuilder& WithName(const FName& name)
            {
                Query.EntityName = name;
                return *this;
            }

            EntityQueryBuilder& RequireAllTags(const EntityQueryFilterTagSet& set)
            {
                Query.TagsAll.AddAll(set);
                Query.TagsAny.RemoveAll(set);
                Query.TagsNone.RemoveAll(set);
                return *this;
            }

            template <class ...T>
            EntityQueryBuilder& RequireAllTags(T&& ...tags) requires ((std::is_same_v<T, FName>) && ...)
            {
                EntityQueryFilterTagSet set;
                (set.Items.push_back(tags), ...);
                return RequireAllTags(set);
            }

            EntityQueryBuilder& RequireAnyTags(const EntityQueryFilterTagSet& set)
            {
                Query.TagsAny.AddAll(set);
                Query.TagsAll.RemoveAll(set);
                Query.TagsNone.RemoveAll(set);
                return *this;
            }

            template <class ...T>
            EntityQueryBuilder& RequireAnyTags(T&& ...tags) requires ((std::is_same_v<T, FName>) && ...)
            {
                EntityQueryFilterTagSet set;
                (set.Items.push_back(tags), ...);
                return RequireAnyTags(set);
            }

            EntityQueryBuilder& RequireNoneTags(const EntityQueryFilterTagSet& set)
            {
                Query.TagsNone.AddAll(set);
                Query.TagsAll.RemoveAll(set);
                Query.TagsAny.RemoveAll(set);
                return *this;
            }

            template <class ...T>
            EntityQueryBuilder& RequireNoneTags(T&& ...tags) requires ((std::is_same_v<T, FName>) && ...)
            {
                EntityQueryFilterTagSet set;
                (set.Items.push_back(tags), ...);
                return RequireNoneTags(set);
            }

            EntityQueryBuilder& RequireAllComponents(const EntityQueryFilterComponentSet& set)
            {
                Query.ComponentsAll.AddAll(set);
                Query.ComponentsAny.RemoveAll(set);
                Query.ComponentsNone.RemoveAll(set);
                return *this;
            }

            template <class ...TComponents>
            EntityQueryBuilder& RequireAllComponents(EComponentAccess access = EComponentAccess::ReadWrite)
            {
                EntityQueryFilterComponentSet set;
                ((set.Items.push_back(std::make_tuple(Underlying_T<TComponents>::StaticTypeName, access))), ...);
                return RequireAllComponents(set);
            }

            EntityQueryBuilder& RequireAnyComponents(const EntityQueryFilterComponentSet& set)
            {
                Query.ComponentsAny.AddAll(set);
                Query.ComponentsAll.RemoveAll(set);
                Query.ComponentsNone.RemoveAll(set);
                return *this;
            }

            template <class ...TComponents>
            EntityQueryBuilder& RequireAnyComponents(EComponentAccess access = EComponentAccess::ReadWrite)
            {
                EntityQueryFilterComponentSet set;
                ((set.Items.push_back(std::make_tuple(Underlying_T<TComponents>::StaticTypeName, access))), ...);
                return RequireAnyComponents(set);
            }

            EntityQueryBuilder& RequireNoneComponents(const EntityQueryFilterComponentSet& set)
            {
                Query.ComponentsNone.AddAll(set);
                Query.ComponentsAll.RemoveAll(set);
                Query.ComponentsAny.RemoveAll(set);
                return *this;
            }

            template <class ...TComponents>
            EntityQueryBuilder& RequireNoneComponents(EComponentAccess access = EComponentAccess::ReadWrite)
            {
                EntityQueryFilterComponentSet set;
                ((set.Items.push_back(std::make_tuple(Underlying_T<TComponents>::StaticTypeName, access))), ...);
                return RequireAnyComponents(set);
            }

            template <class ...TComponents>
            EntityQueryBuilder& Schedule(const TEntityQueryWorkFunc<TComponents...>& func)
            {
                (RequireAllComponents<TComponents>(ComponentAccessFromT<TComponents>::ComponentAccess), ...);
                EntityManager->Schedule(Query, func);
                Query.Reset();
                return *this;
            }

            template <class ...TComponents>
            EntityQueryBuilder& Schedule(const TEntityQueryWorkBufferFunc<TComponents...>& func)
            {
                (RequireAllComponents<TComponents>(ComponentAccessFromT<TComponents>::ComponentAccess), ...);
                EntityManager->Schedule(Query, func);
                Query.Reset();
                return *this;
            }

            template <class ...TComponents>
            EntityQueryBuilder& ScheduleParallel(const TEntityQueryWorkFunc<TComponents...>& func)
            {
                (RequireAllComponents<TComponents>(ComponentAccessFromT<TComponents>::ComponentAccess), ...);
                EntityManager->ScheduleParallel(Query, func);
                Query.Reset();
                return *this;
            }

            template <class ...TComponents>
            EntityQueryBuilder& ScheduleParallel(const TEntityQueryWorkBufferFunc<TComponents...>& func)
            {
                (RequireAllComponents<TComponents>(ComponentAccessFromT<TComponents>::ComponentAccess), ...);
                EntityManager->ScheduleParallel(Query, func);
                Query.Reset();
                return *this;
            }
            
        private:

            TEntityManager* EntityManager;
            EntityQuery Query;
        };
    }
}
