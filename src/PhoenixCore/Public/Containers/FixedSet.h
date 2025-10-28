
#pragma once

#include <functional>

#include "CTZ.h"

namespace Phoenix
{
    template <class TKey, size_t N, class THasher = std::hash<TKey>>
    class TFixedSet
    {
    public:

        static constexpr size_t Capacity = RoundUpPowerOf2(N);

        TFixedSet()
        {
            Reset();
        }
        
        bool IsFull() const
        {
            return Size == Capacity;
        }

        bool IsEmpty() const
        {
            return Size == 0;
        }

        size_t Num() const
        {
            return Size;
        }

        void Reset()
        {
            Size = 0;
            memset(&Items[0], 0, sizeof(TKey) * Capacity);
        }

        bool Insert(const TKey& key)
        {
            PHX_ASSERT(key != 0);

            size_t index = FindSlot(key);
            if (Items[index] != 0)
            {
                // Already in set
                if (Items[index] == key)
                    return true;

                // Set is full
                return false;
            }

            Items[index] = key;
            ++Size;

            return true;
        }

        bool Contains(const TKey& key) const
        {
            size_t index = FindSlot(key);
            return Items[index] == key; 
        }

        size_t FindSlot(const TKey& key) const
        {
            size_t hash = Hash(key);
            size_t index = hash & (Capacity - 1);
            size_t startIndex = index;
            while (Items[index] != 0 && Items[index] != key)
            {
                index = (index + 1) & (Capacity - 1);
                if (index == startIndex)
                    break;
            }
            return index;
        }

    private:

        static size_t Hash(const TKey& key)
        {
            static const THasher hasher;
            return hasher(key);
        }

        TKey Items[Capacity];
        size_t Size = 0;
    };
}
