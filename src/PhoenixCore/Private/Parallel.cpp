
#include "Parallel.h"

#ifndef __EMSCRIPTEN__
// Original implementation would go here for non-web builds
// For web builds, we'll provide simplified stubs

namespace Phoenix
{
    namespace Threading
    {
        // Web-compatible implementation (simplified)
        void SpinWait(uint32 cycles)
        {
            // Simple busy wait without intrinsics
            for (uint32 i = 0; i < cycles; ++i)
            {
                // Empty loop for web compatibility
                volatile uint32 dummy = i;
                (void)dummy;
            }
        }
    }
}

#else
// Web build stubs
namespace Phoenix
{
    namespace Threading  
    {
        void SpinWait(uint32 cycles)
        {
            // No-op for web builds
        }
    }
}

// xatomic.h is Windows-specific, using standard <atomic> and <thread> from Parallel.h

#include "PhoenixCore.h"
#include "Profiling.h"

using namespace Phoenix;

ThreadPool::ThreadPool(const std::string& id, size_t numWorkers, size_t queueCapacity)
    : Id(id)
    , TaskQueue(queueCapacity)
    , NumWorkers(numWorkers)
{
    PHX_ASSERT(numWorkers > 0);
    Threads.reserve(numWorkers);
    for (size_t i = 0; i < numWorkers; ++i)
    {
        Threads.emplace_back([this, i] { Worker(i); });
    }
}

ThreadPool::~ThreadPool()
{
    Shutdown();
}

size_t ThreadPool::GetNumWorkers() const
{
    return NumWorkers;
}

void ThreadPool::Shutdown()
{
    bool expected = false;
    if (Done.compare_exchange_strong(expected, true))
    {
        for (std::thread& thread : Threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }
}

void ThreadPool::Submit(const TTask& task)
{
    size_t attempts = 0;
    while (!TaskQueue.TryEnqueue(task))
    {
        ++attempts;
        if (attempts < 16)
        {
            for (size_t i = 0; i < (1ULL << (attempts > 6 ? 6 : attempts)); ++i)
            {
                _mm_pause();
            }
        }
        else
        {
            std::this_thread::yield();
        }
    }
}

bool ThreadPool::TrySubmit(const TTask& task)
{
    return TaskQueue.TryEnqueue(task);
}

bool ThreadPool::IsEmpty() const
{
    return TaskQueue.IsEmpty();
}

void ThreadPool::WaitIdle()
{
    while (!IsEmpty() || ActiveWorkerCount.load(std::memory_order_acquire) != 0)
    {
        std::this_thread::yield();
    }
}

void ThreadPool::Worker(size_t workerId)
{
    PHX_PROFILE_SET_THREAD_NAME(Id.c_str(), (int32)workerId);

    TTask task;

    while (!Done.load(std::memory_order_acquire))
    {
        if (TaskQueue.TryDequeue(task))
        {
            ActiveWorkerCount.fetch_add(1, std::memory_order_acq_rel);
            task();
            ActiveWorkerCount.fetch_sub(1, std::memory_order_acq_rel);
            continue;
        }

        SpinningWorkerCount.fetch_add(1, std::memory_order_relaxed);

        // Spin with exponential backoff
        size_t spins = 0;
        while (!Done.load(std::memory_order_acquire))
        {
            if (TaskQueue.TryDequeue(task))
            {
                ActiveWorkerCount.fetch_add(1, std::memory_order_acq_rel);
                task();
                ActiveWorkerCount.fetch_sub(1, std::memory_order_acq_rel);
                break;
            }
            if (spins < 8)
            {
                for (size_t i = 0; i < (1ULL << spins); ++i)
                {
                    _mm_pause();
                }
            }
            else
            {
                std::this_thread::yield();
            }
            ++spins;
        }

        SpinningWorkerCount.fetch_sub(1, std::memory_order_relaxed);
    }

    while (TaskQueue.TryDequeue(task))
    {
        ActiveWorkerCount.fetch_add(1, std::memory_order_acq_rel);
        task();
        ActiveWorkerCount.fetch_sub(1, std::memory_order_acq_rel);
    }
}

#endif


