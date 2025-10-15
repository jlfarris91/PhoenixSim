
#pragma once

#include <atomic>
#include <memory>

namespace Phoenix
{
    template <class T>
    class TMPMCQueue
    {
    public:

        TMPMCQueue(size_t capacity)
            : Capacity(capacity)
            , Mask(capacity - 1)
            , Buffer(new Cell[capacity])
            , EnqueuePos(0)
            , DequeuePos(0)
        {
            for (size_t i = 0; i < Capacity; ++i)
            {
                Buffer[i].sequence.store(i, std::memory_order_relaxed);
            }
        }

        size_t GetCapacity() const { return Capacity; }

        bool TryEnqueue(const T& item)
        {
            size_t pos = EnqueuePos.load(std::memory_order_relaxed);
            while (true)
            {
                Cell& cell = Buffer[pos & Mask];
                size_t seq = cell.sequence.load(std::memory_order_relaxed);
                intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
                if (diff == 0)
                {
                    if (EnqueuePos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        cell.Data = item;
                        cell.sequence.store(pos + 1, std::memory_order_release);
                        return true;
                    }
                }
                else if (diff < 0)
                {
                    return false;
                }
                else
                {
                    pos = EnqueuePos.load(std::memory_order_relaxed);
                }
            }
        }

        bool TryDequeue(T& outItem)
        {
            size_t pos = DequeuePos.load(std::memory_order_relaxed);
            while (true)
            {
                Cell& cell = Buffer[pos & Mask];
                size_t seq = cell.sequence.load(std::memory_order_relaxed);
                intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
                if (diff == 0)
                {
                    if (DequeuePos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        outItem = std::move(cell.Data);
                        cell.sequence.store(pos + Capacity, std::memory_order_release);
                        return true;
                    }
                }
                else if (diff < 0)
                {
                    return false;
                }
                else
                {
                    pos = DequeuePos.load(std::memory_order_relaxed);
                }
            }
        }

        bool IsEmpty() const
        {
            return EnqueuePos.load(std::memory_order_relaxed) == DequeuePos.load(std::memory_order_relaxed);
        }

    private:

        struct Cell
        {
            T Data;
            std::atomic<size_t> sequence;
        };

        size_t Capacity;
        size_t Mask;
        std::unique_ptr<Cell[]> Buffer;
        std::atomic<size_t> EnqueuePos;
        std::atomic<size_t> DequeuePos;
    };
}
