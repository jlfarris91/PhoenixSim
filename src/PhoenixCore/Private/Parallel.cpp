
#include "Parallel.h"

// xatomic.h is Windows-specific, using standard <atomic> and <thread> from Parallel.h

#include "Platform.h"
#include "Profiling.h"

using namespace Phoenix;

bool TaskHandle::IsCompleted() const
{
    return bIsCompleted.load(std::memory_order_acquire);
}

bool TaskHandle::WaitForCompleted(clock_t maxWaitTime) const
{
    clock_t startTime = PHX_CLOCK();
    while (!IsCompleted())
    {
        std::this_thread::yield();
        if (maxWaitTime && (PHX_CLOCK() - startTime) > maxWaitTime)
        {
            return false;
        }
    }
    return true;
}

void TaskHandle::OnCompleted(const std::function<void()>& fn)
{
    OnCompletedFunc = fn;
    if (IsCompleted())
    {
        OnCompletedFunc();
    }
}

Task::Task() = default;

Task::Task(const std::function<void()>& work)
    : WorkFunc(work)
{
}

void Task::operator()() const
{
    Handle->bIsCompleted.store(false, std::memory_order_release);
    WorkFunc();
    Handle->bIsCompleted.store(true, std::memory_order_release);
    if (Handle->OnCompletedFunc)
    {
        Handle->OnCompletedFunc();
    }
}

bool Task::WaitAll(const std::vector<TSharedPtr<TaskHandle>>& handles, clock_t maxWaitTime)
{
    clock_t startTime = PHX_CLOCK();
   
    for (;;)
    {
        bool done = true;
        for (const TSharedPtr<TaskHandle>& handle : handles)
        {
            if (!handle->IsCompleted())
            {
                done = false;
                break;
            }
        }

        if (done)
        {
            return true;
        }

        if (maxWaitTime && (PHX_CLOCK() - startTime) > maxWaitTime)
        {
            return false;
        }

        std::this_thread::yield();
    }
}

bool Task::WaitAny(const std::vector<TSharedPtr<TaskHandle>>& handles, clock_t maxWaitTime)
{
    clock_t startTime = PHX_CLOCK();

    for (;;)
    {
        for (const TSharedPtr<TaskHandle>& handle : handles)
        {
            if (handle->IsCompleted())
            {
                return true;
            }
        }

        if (maxWaitTime && (PHX_CLOCK() - startTime) > maxWaitTime)
        {
            return false;
        }
    }
}

ThreadPool::ThreadPool(std::string id, uint32 numWorkers, uint32 queueCapacity)
    : Id(std::move(id))
    , TaskQueue(queueCapacity)
    , NumWorkers(numWorkers)
{
    PHX_ASSERT(numWorkers > 0);
    Threads.reserve(numWorkers);
    for (uint32 i = 0; i < numWorkers; ++i)
    {
        Threads.emplace_back([this, i] { Worker(i); });
    }
}

ThreadPool::~ThreadPool()
{
    Shutdown();
}

uint32 ThreadPool::GetNumWorkers() const
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

TSharedPtr<TaskHandle> ThreadPool::Submit(const Task& task)
{
    auto handle = MakeShared<TaskHandle>();

    Task taskWithHandle = task;
    taskWithHandle.Handle = handle;

    uint32 attempts = 0;
    while (!TaskQueue.TryEnqueue(taskWithHandle))
    {
        ++attempts;
        if (attempts < 16)
        {
            for (uint32 i = 0; i < (1ULL << (attempts > 6 ? 6 : attempts)); ++i)
            {
                PHX_THREAD_PAUSE();
            }
        }
        else
        {
            std::this_thread::yield();
        }
    }

    return handle;
}

TSharedPtr<TaskHandle> ThreadPool::Submit(const std::function<void()>& work)
{
    return Submit(Task(work));
}

bool ThreadPool::IsEmpty() const
{
    return TaskQueue.IsEmpty();
}

bool ThreadPool::WaitIdle(clock_t maxWaitTime) const
{
    clock_t startTime = PHX_CLOCK();
    while (!IsEmpty() || ActiveWorkerCount.load(std::memory_order_acquire) != 0)
    {
        std::this_thread::yield();
        if (maxWaitTime && (PHX_CLOCK() - startTime) > maxWaitTime)
        {
            return false;
        }
    }
    return true;
}

void ThreadPool::Worker(uint32 workerId)
{
    PHX_PROFILE_SET_THREAD_NAME(Id.c_str(), (int32)workerId);

    Task task;

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
        uint32 spins = 0;
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
                for (uint32 i = 0; i < (1ULL << spins); ++i)
                {
                    PHX_THREAD_PAUSE();
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

TMap<uint32, TSharedPtr<TaskQueue>> gTaskQueues;
std::mutex gTaskQueueMutex;

TaskQueue::TaskQueue(uint32 id)
    : Id (id)
{
}

TSharedPtr<TaskQueue> TaskQueue::CreateTaskQueue(uint32 id)
{
    std::scoped_lock lock(gTaskQueueMutex);

    auto iter = gTaskQueues.find(id);
    PHX_ASSERT(iter == gTaskQueues.end());

    TSharedPtr<TaskQueue> taskQueue = MakeShared<TaskQueue>(id);
    gTaskQueues[id] = taskQueue;
    return taskQueue;
}

TSharedPtr<TaskQueue> TaskQueue::GetTaskQueue(uint32 id)
{
    std::scoped_lock lock(gTaskQueueMutex);

    auto iter = gTaskQueues.find(id);
    if (iter != gTaskQueues.end())
    {
        return iter->second;
    }

    return nullptr;
}

bool TaskQueue::ReleaseTaskQueue(uint32 id)
{
    std::scoped_lock lock(gTaskQueueMutex);

    auto iter = gTaskQueues.find(id);
    if (iter != gTaskQueues.end())
    {
        gTaskQueues.erase(iter);
        return true;
    }
    return false;
}

void TaskQueue::Enqueue(const Task& task)
{
    Tasks.push_back({ task });
}

void TaskQueue::Enqueue(const std::function<void()>& work)
{
    Enqueue(Task(work));
}

void TaskQueue::Enqueue(const std::vector<Task>& tasks)
{
    Tasks.push_back(tasks);
}

void TaskQueue::Flush()
{
    bIsCompleted.store(false, std::memory_order_release);

    ThreadPool* threadPool = GetThreadPool();

    for (const std::vector<Task>& tasks : Tasks)
    {
        std::vector<TSharedPtr<TaskHandle>> handles;
        handles.reserve(tasks.size());
        
        for (const Task& task : tasks)
        {
            handles.push_back(threadPool->Submit(task));
        }

        Task::WaitAll(handles);
    }

    Complete();
}

void TaskQueue::Complete()
{
    Tasks.clear();
    CurrTaskIndex = 0;
    bIsCompleted.store(true, std::memory_order_release);
}

TUniquePtr<ThreadPool> gThreadPool;

bool Phoenix::HasThreadPool()
{
    return gThreadPool != nullptr;
}

ThreadPool* Phoenix::GetThreadPool()
{
    return gThreadPool.get();
}

void Phoenix::SetThreadPool(const std::string& id, uint32 numWorkers, uint32 queueCapacity)
{
    gThreadPool = MakeUnique<ThreadPool>(id, numWorkers, queueCapacity);
}

void Phoenix::DestroyThreadPool()
{
    (void)gThreadPool.release();
}
