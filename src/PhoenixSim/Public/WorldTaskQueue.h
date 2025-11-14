
#pragma once

#include "Parallel.h"
#include "Worlds.h"

namespace Phoenix
{
    struct WorldTaskQueue
    {
        using TWorldTaskFunc = std::function<void(WorldRef)>;
        using TParallelRangeFunc = std::function<void(WorldRef, uint32, uint32)>;

        static void Schedule(WorldRef world, TTaskFunc&& func)
        {
            TSharedPtr<TaskQueue> taskQueue = TaskQueue::GetTaskQueue((uint32)world.GetName());
            taskQueue->Enqueue(std::move(func));
        }

        static void Schedule(WorldRef world, const Task& task)
        {
            TSharedPtr<TaskQueue> taskQueue = TaskQueue::GetTaskQueue((uint32)world.GetName());
            taskQueue->Enqueue(task);
        }

        static void Schedule(WorldRef world, TWorldTaskFunc&& func)
        {
            auto worldPtr = &world;
            TTaskFunc wrapper = [=] { func(*worldPtr); };
            Schedule(world, std::move(wrapper));
        }

        template <class _Fx, class ...TArgs>
        static void Schedule(WorldRef world, _Fx&& fx, TArgs&&... args)
        {
            using namespace std::placeholders;
            TWorldTaskFunc wrapper = std::bind(std::forward<_Fx>(fx), _1, std::forward<TArgs>(args)...);
            Schedule(world, std::move(wrapper));
        }

        static void dfsdf(WorldRef, int)
        {
        }

        void asdf()
        {
            auto a = [](WorldRef, int)
            {
                
            };
            TWorldTaskFunc b = std::bind(a, std::placeholders::_1, 123);
            TWorldTaskFunc c = std::bind(std::forward<void(*)(WorldRef, int)>(&dfsdf), std::placeholders::_1, 123);
        }

        template <class ...TArgs>
        static void Schedule(WorldRef world, const std::function<void(WorldRef, TArgs...)>& func, TArgs&& ...args)
        {
            TSharedPtr<TaskQueue> taskQueue = TaskQueue::GetTaskQueue((uint32)world.GetName());
            auto worldPtr = &world;
            taskQueue->Enqueue([=]
            {
                func(*worldPtr, std::forward<TArgs>(args)...);
            });
        }

        static void ScheduleParallelRange(WorldRef world, uint32 total, uint32 minRange, TParallelRangeFunc&& func)
        {
            auto worldPtr = &world;
            TSharedPtr<TaskQueue> taskQueue = TaskQueue::GetTaskQueue((uint32)world.GetName());

            std::vector<Task>& taskGroup = taskQueue->BeginGroup();

            uint32 desiredRange = total / taskQueue->GetNumWorkers();
            uint32 actualRange = desiredRange < minRange ? minRange : desiredRange;
            uint32 start = 0;
            while (start != total)
            {
                uint32 len = actualRange;
                len = len > (total - start) ? (total - start) : len;
                taskGroup.emplace_back([=] { func(*worldPtr, start, len); });
                start += len;
            }

            taskQueue->EndGroup();
        }

        template <class _Fx, class ...TArgs>
        static void ScheduleParallelRange(WorldRef world, uint32 total, uint32 minRange, _Fx&& fx, TArgs&&... args)
        {
            using namespace std::placeholders;
            TParallelRangeFunc wrapper = std::bind(std::forward<_Fx>(fx), _1, _2, _3, std::forward<TArgs>(args)...);
            ScheduleParallelRange(world, total, minRange, std::move(wrapper));
        }
        
        static void Flush(WorldRef world)
        {
            PHX_PROFILE_ZONE_SCOPED;

            TSharedPtr<TaskQueue> taskQueue = TaskQueue::GetTaskQueue((uint32)world.GetName());

            // Submit any pending jobs and pause the thread until they finish.
            taskQueue->Flush();
        }
    };
}
