
#pragma once

#include <cstring>      // For memset
#include <functional>

#include "PlatformTypes.h"

namespace Phoenix
{
    template <class TKey, class TValue, size_t N, class THasher = std::hash<TKey>>
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
            memset(&Items[0], 0, sizeof(Element) * N);
        }

        bool Insert(const TKey& key, const TValue& value = {})
        {
            size_t slot = FindSlot(key);

            if (Items[slot].Key != 0 && Items[slot].Key != key)
            {
                // Map is full
                return false;
            }

            Items[slot].Key = key;
            Items[slot].Value = value;
            return true;
        }

        template <class ...TArgs>
        bool Emplace(const TKey& key, const TArgs&... args)
        {
            size_t slot = FindSlot(key);

            if (Items[slot].Key != 0 && Items[slot].Key != key)
            {
                // Map is full
                return false;
            }

            Items[slot].Key = key;
            new (&Items[slot].Value) TValue(args...);

            return true;
        }

        TValue& InsertDefaulted_GetRef(const TKey& key)
        {
            size_t slot = FindSlot(key);

            if (Items[slot].Key != 0)
            {
                // KVP already exists
                if (Items[slot].Key == key)
                {
                    Items[slot].Value = TValue();
                    return Items[slot].Value;
                }

                // Map is full
                PHX_ASSERT(false);
            }

            Items[slot].Key = key;
            Items[slot].Value = TValue();

            return Items[slot].Value;
        }

        bool Contains(const TKey& key) const
        {
            size_t slot = FindSlot(key);
            return Items[slot].Key == key;
        }

        TValue* GetPtr(const TKey& key)
        {
            size_t slot = FindSlot(key);
            if (Items[slot].Key == key)
            {
                return &Items[slot].Value;
            }
            return nullptr;
        }

        const TValue* GetPtr(const TKey& key) const
        {
            size_t slot = FindSlot(key);
            if (Items[slot].Key == key)
            {
                return &Items[slot].Value;
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

        TValue* FindOrAdd(const TKey& key, const TValue& value)
        {
            size_t slot = FindSlot(key);

            if (Items[slot].Key != 0)
            {
                // KVP already exists
                if (Items[slot].Key == key)
                {
                    Items[slot].Value = value;
                    return &Items[slot].Value;
                }

                // Map is full
                return nullptr;
            }

            Items[slot] = key;
            Items[slot] = value;
            ++Size;

            return &Items[slot].Value;
        }

        TValue* FindOrAddDefaulted(const TKey& key)
        {
            size_t slot = FindSlot(key);

            if (Items[slot].Key != 0)
            {
                // KVP already exists
                if (Items[slot].Key == key)
                {
                    Items[slot].Value = TValue();
                    return &Items[slot].Value;
                }

                // Map is full
                return nullptr;
            }

            Items[slot] = key;
            Items[slot] = TValue();
            ++Size;

            return &Items[slot].Value;
        }

        // See https://en.wikipedia.org/wiki/Open_addressing
        bool Remove(const TKey& key)
        {
            size_t i = FindSlot(key);

            // Slot is not occupied
            if (Items[i].Key == 0)
            {
                return false;
            }

            // Mark slot as unoccupied
            Items[i].Key = 0;

            PHX_ASSERT(Size > 0);
            --Size;

            // Attempt to fill item
            size_t j = i;
            for (;;)
            {
                j = (j + 1) % Capacity;
                if (Items[j].Key == 0)
                {
                    break;
                }

                size_t k = Hash(Items[k].Key);

                // determine if k lies cyclically in (i,j]
                // i ≤ j: |    i..k..j    |
                // i > j: |.k..j     i....| or |....j     i..k.|
                if (i <= j)
                {
                    if (i < k && k <= j)
                    {
                        continue;
                    }
                }
                else if (k <= j || i < k)
                {
                    continue;
                }

                // Move slot[j] into slot[i]
                Items[i] = Items[j];

                // Mark slot[j] as unoccupied
                Items[j].Key = 0;

                i = j;
            }

            return true;
        }

    private:

        static size_t Hash(const TKey& key)
        {
            static THasher hasher;
            return hasher(key) % N;
        }

        size_t FindSlot(const TKey& key) const
        {
            size_t hash = Hash(key);
            size_t index = hash & (Capacity - 1);
            size_t startIndex = index;
            while (Items[index].Key != 0 && Items[index].Key != key)
            {
                index = (index + 1) & (Capacity - 1);
                if (index == startIndex)
                    break;
            }
            return index;
        }

        struct Element
        {
            TKey Key = {};
            TValue Value = {};
        };

        Element Items[N];
        size_t Size = 0;
    };
}
