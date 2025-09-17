#pragma once
#include "PhoenixSim.h"

namespace Phoenix
{
    template <class TKey, size_t N>
    class TFastSet
    {
    public:
        bool IsFull() const
        {
            return Size == N;
        }

        bool IsEmpty() const
        {
            return Size == 0;
        }

        void Reset()
        {
            Size = 0;
            memset(&Table[0], 0, sizeof(Element) * N);
        }

        bool Insert(const TKey& key)
        {
            size_t idx = Hash(key);
            for (int i = 0; i < N; ++i)
            {
                size_t probe = (idx + i) % N;
                if (!Table[probe].Occupied || Table[probe].Key == key)
                {
                    Table[probe].Occupied = true;
                    Table[probe].Key = key;
                    ++Size;
                    return true;
                }
            }
            return false;
        }

        bool Contains(const TKey& key) const
        {
            size_t idx = Hash(key);
            for (int i = 0; i < N; ++i)
            {
                size_t probe = (idx + i) % N;
                if (!Table[probe].Occupied)
                {
                    return false;
                }
                if (Table[probe].Key == key)
                {
                    return true;
                }
            }
            return false;
        }

    private:

        size_t Hash(const TKey& key) const
        {
            return std::hash<TKey>{}(key) % N;
        }

        struct Element
        {
            uint8 Occupied = 0;
            TKey Key;
        };

        Element Table[N];
        size_t Size = 0;
    };
}
