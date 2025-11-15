
#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "Platform.h"
#include "Containers/MPMCQueue.h"

namespace Phoenix
{
    struct PHOENIXCORE_API TaskHandle
    {
        bool IsCompleted() const;
        bool WaitForCompleted(std::chrono::milliseconds maxWaitTime = std::chrono::milliseconds(0)) const;
        void OnCompleted(std::function<void()>&& fn);

    private:
        friend class Task;
        std::function<void()> OnCompletedFunc;
        std::atomic<bool> bIsCompleted;
    };

    using TTaskFunc = std::function<void()>;
    
    class PHOENIXCORE_API Task
    {
    public:

        Task();
        Task(const Task& other) = default;
        Task(Task&& other);
        Task(TTaskFunc&& work);

        void operator()() const;

        Task& operator=(const Task& other) = default;
        Task& operator=(Task&& other) = default;

        static bool WaitAll(const std::vector<TSharedPtr<TaskHandle>>& handles, std::chrono::milliseconds maxWaitTime = std::chrono::milliseconds(0));
        static bool WaitAny(const std::vector<TSharedPtr<TaskHandle>>& handles, std::chrono::milliseconds maxWaitTime = std::chrono::milliseconds(0));

    private:

        friend class ThreadPool;

        TTaskFunc WorkFunc;
        TSharedPtr<TaskHandle> Handle;
    };

    PHOENIXCORE_API bool HasThreadPool();
    PHOENIXCORE_API ThreadPool* GetThreadPool();
    PHOENIXCORE_API void SetThreadPool(const std::string& id, uint32 numWorkers, uint32 queueCapacity = 1024);
    PHOENIXCORE_API void DestroyThreadPool();

    class PHOENIXCORE_API ThreadPool
    {
    public:
        ThreadPool(std::string id, uint32 numWorkers, uint32 queueCapacity = 1024);
        ~ThreadPool();

        uint32 GetNumWorkers() const;

        void Shutdown();

        TSharedPtr<TaskHandle> Submit(const Task& task);
        TSharedPtr<TaskHandle> Submit(TTaskFunc&& work);

        bool IsEmpty() const;
        bool WaitIdle(std::chrono::milliseconds maxWaitTime = std::chrono::milliseconds(0)) const;

    private:

        void Worker(uint32 workerId);

        std::string Id;
        std::vector<std::thread> Threads;
        TMPMCQueue<Task> TaskQueue;
        std::atomic<bool> Done = false;
        std::atomic<uint32_t> ActiveWorkerCount;
        std::atomic<uint32_t> SpinningWorkerCount;
        uint32 NumWorkers;
    };

    class PHOENIXCORE_API TaskQueue
    {
    public:

        TaskQueue(uint32 id, ThreadPool* threadPool = Phoenix::GetThreadPool());

        static TSharedPtr<TaskQueue> CreateTaskQueue(uint32 id);
        static TSharedPtr<TaskQueue> GetTaskQueue(uint32 id);
        static bool ReleaseTaskQueue(uint32 id);

        uint32 GetId() const;
        ThreadPool* GetThreadPool() const;
        uint32 GetNumWorkers() const;

        void Enqueue(Task&& task);
        void Enqueue(TTaskFunc&& work);
        void Enqueue(std::vector<Task>&& tasks);

        std::vector<Task>& BeginGroup(uint32 size = 0);
        void EndGroup();

        void Flush();

    private:

        void Complete();

        uint32 Id = 0;
        std::vector<std::vector<Task>> Tasks;
        std::atomic<uint32> CurrTaskIndex;
        std::atomic<bool> bIsCompleted = false;
        ThreadPool* ThreadPool;
    };

    template <class TThreadPool, class TJob>
    void ParallelForEach(TThreadPool& pool, uint32 num, const TJob& job)
    {
        for (uint32 i = 0; i < num; ++i)
        {
            pool.Submit([=] { job(i); });
        }
        pool.WaitIdle();
    }

    template <class TJob>
    void ParallelForEach(uint32 num, const TJob& job)
    {
        if (ThreadPool* threadPool = GetThreadPool())
        {
            ParallelForEach(*threadPool, num, job);
            return;
        }

        // No thread pool? Just run synchronously.
        for (uint32 i = 0; i < num; ++i)
        {
            job(i);
        }
    }

    template <class TThreadPool, class TJob>
    void ParallelRange(TThreadPool& pool, uint32 total, uint32 minRange, const TJob& job)
    {
        uint32 desiredRange = total / pool.GetNumWorkers();
        uint32 actualRange = desiredRange < minRange ? minRange : desiredRange;
        uint32 start = 0;
        while (start != total)
        {
            uint32 len = actualRange;
            len = len > (total - start) ? (total - start) : len;
            pool.Submit([=] { job(start, len); });
            start += len;
        }
        pool.WaitIdle();
    }

    template <class TJob>
    void ParallelRange(uint32 total, uint32 minRange, const TJob& job)
    {
        if (ThreadPool* threadPool = GetThreadPool())
        {
            ParallelRange(*threadPool, total, minRange, job);
            return;
        }

        // No thread pool? Just run synchronously.
        uint32 desiredRange = total;
        uint32 actualRange = desiredRange < minRange ? minRange : desiredRange;
        uint32 start = 0;
        while (start != total)
        {
            uint32 len = actualRange;
            len = len > (total - start) ? (total - start) : len;
            job(start, len);
            start += len;
        }
    }
}
