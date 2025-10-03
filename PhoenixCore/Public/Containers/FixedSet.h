
#pragma once

#include <functional>

namespace Phoenix
{
    template <class TKey, size_t N, class THash = std::hash<TKey>>
    class TFixedSet
    {
    public:

        static constexpr size_t Capacity = N;

        TFixedSet()
        {
            Reset();
        }
        
        bool IsFull() const
        {
            return Size == N;
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
            memset(&Table[0], 0, sizeof(Element) * N);
        }

        bool Insert(const TKey& key, size_t* outProbeLen = nullptr)
        {
            if (IsFull())
            {
                return false;
            }

            size_t idx = Hash(key);
            for (size_t i = 0; i < N; ++i)
            {
                size_t probe = (idx + i) % N;
                if (Table[probe].Key == key)
                {
                    Table[probe].Occupied += 1;
                    return true;
                }
                if (!Table[probe].Occupied)
                {
                    Table[probe].Occupied += 1;
                    Table[probe].Key = key;
                    ++Size;
                    return true;
                }
                if (outProbeLen) (*outProbeLen)++;
            }

            return false;
        }

        bool Contains(const TKey& key, size_t* outProbeLen = nullptr) const
        {
            size_t idx = Hash(key);
            for (size_t i = 0; i < N; ++i)
            {
                size_t probe = (idx + i) % N;
                if (Table[probe].Key == key)
                {
                    return true;
                }
                if (!Table[probe].Occupied)
                {
                    return false;
                }
                if (outProbeLen) (*outProbeLen)++;
            }
            return false;
        }

    private:

        static size_t Hash(const TKey& key)
        {
            return THash{}(key) % N;
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
