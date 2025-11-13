
#pragma once

#include "ArchetypeList.h"
#include "EntityId.h"
#include "Worlds.h"
#include "ArchetypeManager.h"

namespace Phoenix
{
    namespace ECS
    {
        struct IEntityJobBase
        {
            virtual ~IEntityJobBase() = default;

            const EntityQuery& GetQuery() const
            {
                return Query;
            }
            
            virtual void Execute(WorldRef world, ArchetypeList& list, uint32 startIndex)
            {
                World = &world;
            }

        protected:

            WorldPtr World = nullptr;
            EntityQuery Query;
        };

        template <class ...TComponents>
        struct IEntityJob : IEntityJobBase
        {
            IEntityJob()
            {
                EntityQueryBuilder builder;
                builder.RequireAllComponents<TComponents...>();
                Query = builder.GetQuery();
            }

            void Execute(WorldRef world, ArchetypeList& list, uint32 startIndex) final
            {
                IEntityJobBase::Execute(world, list, startIndex);

                auto span = EntityComponentSpan<TComponents...>::FromList(list, startIndex);
                for (const auto& tuple : span)
                {
                    std::apply(&Execute, tuple);
                }
            }
            
            virtual void Execute(EntityId entityId, TComponents&& ...components) = 0;
        };

        template <class ...TComponents>
        struct IBufferJob : IEntityJobBase
        {
            IBufferJob()
            {
                EntityQueryBuilder builder;
                builder.RequireAllComponents<TComponents...>();
                Query = builder.GetQuery();
            }
            
            void Execute(WorldRef world, ArchetypeList& list, uint32 startIndex) final
            {
                IEntityJobBase::Execute(world, list, startIndex);

                auto span = EntityComponentSpan<TComponents...>::FromList(list, startIndex);
                Execute(span);
            }
            
            virtual void Execute(const EntityComponentSpan<TComponents...>&) = 0;
        };
        
        template <class ...TComponents>
        using TJobFunction = TFunction<void(WorldRef, EntityId, TComponents...)>;

        template <class ...TComponents>
        struct FunctionJob : IEntityJob<TComponents...>
        {
            FunctionJob(const TJobFunction<TComponents...>& func) : Func(func) {}

            void Execute(EntityId entityId, TComponents&& ...components) override
            {
                Func(*this->World, entityId, std::forward<TComponents>(components)...);
            }

            TJobFunction<TComponents...> Func;
        };

        template <class ...TComponents>
        using TBufferJobFunction = TFunction<void(WorldRef world, const EntityComponentSpan<TComponents...>&)>;

        template <class ...TComponents>
        struct FunctionBufferJob : IBufferJob<TComponents...>
        {
            FunctionBufferJob(const TBufferJobFunction<TComponents...>& func) : Func(func) {}

            void Execute(const EntityComponentSpan<TComponents...>& span) override
            {
                Func(*this->World, span);
            }

            TBufferJobFunction<TComponents...> Func;
        };
    }
}
