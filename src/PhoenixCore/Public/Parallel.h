
#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "DLLExport.h"
#include "Containers/MPMCQueue.h"

namespace Phoenix
{
    class PHOENIXCORE_API ThreadPool
    {
    public:
        using TTask = std::function<void()>;

        ThreadPool(const std::string& id, size_t numWorkers, size_t queueCapacity = 1024);
        ~ThreadPool();

        size_t GetNumWorkers() const;

        void Shutdown();

        void Submit(const TTask& task);
        bool TrySubmit(const TTask& task);

        bool IsEmpty() const;
        void WaitIdle();

    private:

        void Worker(size_t workerId);

        std::string Id;
        std::vector<std::thread> Threads;
        TMPMCQueue<TTask> TaskQueue;
        std::atomic<bool> Done = false;
        std::atomic<uint32_t> ActiveWorkerCount;
        std::atomic<uint32_t> SpinningWorkerCount;
        size_t NumWorkers;
    };

    template <class TThreadPool, class TJob>
    void ParallelForEach(TThreadPool& pool, size_t num, const TJob& job)
    {
        for (size_t i = 0; i < num; ++i)
        {
            pool.Submit([=] { job(i); });
        }
        pool.WaitIdle();
    }

    template <class TThreadPool, class TJob>
    void ParallelRange(TThreadPool& pool, size_t total, size_t minRange, const TJob& job)
    {
        size_t desiredRange = total / pool.GetNumWorkers();
        size_t actualRange = desiredRange < minRange ? minRange : desiredRange;
        size_t start = 0;
        while (start != total)
        {
            size_t len = actualRange;
            len = len > (total - start) ? (total - start) : len;
            pool.Submit([=] { job(start, len); });
            start += len;
        }
        pool.WaitIdle();
    }
}
