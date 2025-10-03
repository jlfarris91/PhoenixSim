
#pragma once

#include <functional>

#include "PlatformTypes.h"

namespace Phoenix
{
    template <class TKey, class TValue, size_t N, class THash = std::hash<TKey>>
    class TFixedMap
    {
    public:

        static constexpr size_t Capacity = N;

        TFixedMap()
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

        bool Insert(const TKey& key, const TValue& value = {})
        {
            if (IsFull())
            {
                return false;
            }

#if DEBUG
            ProbeLen = 0;
#endif

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
                    Table[probe].Value = value;
                    ++Size;
                    return true;
                }
#if DEBUG
                ++ProbeLen;
#endif
            }

            return false;
        }

        TValue& InsertDefaulted_GetRef(const TKey& key)
        {
            Insert(key, {});
            return Get(key);
        }

        bool Contains(const TKey& key) const
        {
#if DEBUG
            ProbeLen = 0;
#endif

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
#if DEBUG
                ++ProbeLen;
#endif
            }

            return false;
        }

        TValue* GetPtr(const TKey& key)
        {
#if DEBUG
            ProbeLen = 0;
#endif

            size_t idx = Hash(key);
            for (size_t i = 0; i < N; ++i)
            {
                size_t probe = (idx + i) % N;
                if (Table[probe].Key == key)
                {
                    return &Table[probe].Value;
                }
                if (!Table[probe].Occupied)
                {
                    break;
                }
#if DEBUG
                ++ProbeLen;
#endif
            }

            return nullptr;
        }

        const TValue* GetPtr(const TKey& key) const
        {
#if DEBUG
            ProbeLen = 0;
#endif

            size_t idx = Hash(key);
            for (size_t i = 0; i < N; ++i)
            {
                size_t probe = (idx + i) % N;
                if (Table[probe].Key == key)
                {
                    return &Table[probe].Value;
                }
                if (!Table[probe].Occupied)
                {
                    break;
                }
#if DEBUG
                ++ProbeLen;
#endif
            }

            return nullptr;
        }

        TValue& Get(const TKey& key)
        {
            return *GetPtr(key);
        }

        const TValue& Get(const TKey& key) const
        {
            return *GetPtr(key);
        }

        TValue& operator[](const TKey& key)
        {
            return Get(key);
        }

        const TValue& operator[](const TKey& key) const
        {
            return Get(key);
        }

#if DEBUG
        // Returns the probe length of the last operation.
        uint32 GetProbeLen() const
        {
            return ProbeLen;
        }
#endif

    private:

        static size_t Hash(const TKey& key)
        {
            return THash{}(key) % N;
        }

        struct Element
        {
            uint8 Occupied = 0;
            TKey Key;
            TValue Value;
        };

        Element Table[N];
        size_t Size = 0;

#if DEBUG
        mutable uint32 ProbeLen = 0;
#endif
    };
}
