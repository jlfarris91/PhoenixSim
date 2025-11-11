
#pragma once

#include <cstring>      // For memset
#include <functional>

#include "Platform.h"

namespace Phoenix
{
    template <class TKey, class TValue, size_t N, class THasher = std::hash<TKey>>
    class TFixedMap
    {
    public:

        using TElement = TPair<TKey, TValue>;
        static constexpr size_t Capacity = N;

        TFixedMap()
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
            memset(&Items[0], 0, sizeof(TElement) * Capacity);
        }

        bool Insert(const TKey& key, const TValue& value = {})
        {
            size_t slot = FindSlot(key);

            if (Items[slot].first != 0)
            {
                // KVP already exists
                if (Items[slot].first == key)
                {
                    Items[slot].second = value;
                    return true;
                }

                // Map is full
                return false;
            }

            Items[slot].first = key;
            Items[slot].second = value;
            ++Size;

            return true;
        }

        template <class ...TArgs>
        bool Emplace(const TKey& key, const TArgs&... args)
        {
            size_t slot = FindSlot(key);

            if (Items[slot].first != 0)
            {
                // KVP already exists
                if (Items[slot].first == key)
                {
                    new (&Items[slot].second) TValue(args...);
                    return true;
                }

                // Map is full
                return false;
            }

            Items[slot].first = key;
            new (&Items[slot].second) TValue(args...);
            ++Size;

            return true;
        }

        TValue& InsertDefaulted_GetRef(const TKey& key)
        {
            size_t slot = FindSlot(key);

            if (Items[slot].first != 0)
            {
                // KVP already exists
                if (Items[slot].first == key)
                {
                    Items[slot].second = TValue();
                    return Items[slot].second;
                }

                // Map is full
                PHX_ASSERT(false);
            }

            Items[slot].first = key;
            Items[slot].second = TValue();
            ++Size;

            return Items[slot].second;
        }

        bool Contains(const TKey& key) const
        {
            size_t slot = FindSlot(key);
            return Items[slot].first == key;
        }

        TValue* GetPtr(const TKey& key)
        {
            size_t slot = FindSlot(key);
            if (Items[slot].first == key)
            {
                return &Items[slot].second;
            }
            return nullptr;
        }

        const TValue* GetPtr(const TKey& key) const
        {
            size_t slot = FindSlot(key);
            if (Items[slot].first == key)
            {
                return &Items[slot].second;
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

            if (Items[slot].first != 0)
            {
                // KVP already exists
                if (Items[slot].first == key)
                {
                    Items[slot].second = value;
                    return &Items[slot].second;
                }

                // Map is full
                return nullptr;
            }

            Items[slot].first = key;
            Items[slot].second = value;
            ++Size;

            return &Items[slot].second;
        }

        TValue* FindOrAddDefaulted(const TKey& key)
        {
            size_t slot = FindSlot(key);

            if (Items[slot].first != 0)
            {
                // KVP already exists
                if (Items[slot].first == key)
                {
                    Items[slot].second = TValue();
                    return &Items[slot].second;
                }

                // Map is full
                return nullptr;
            }

            Items[slot].first = key;
            Items[slot].second = TValue();
            ++Size;

            return &Items[slot].second;
        }

        // See https://en.wikipedia.org/wiki/Open_addressing
        bool Remove(const TKey& key)
        {
            size_t i = FindSlot(key);

            // Slot is not occupied
            if (Items[i].first == 0)
            {
                return false;
            }

            // Mark slot as unoccupied
            Items[i].first = 0;

            PHX_ASSERT(Size > 0);
            --Size;

            // Attempt to fill item
            size_t j = i;
            for (;;)
            {
                j = (j + 1) % Capacity;
                if (Items[j].first == 0)
                {
                    break;
                }

                size_t k = Hash(Items[j].first);

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
                Items[j].first = 0;

                i = j;
            }

            return true;
        }

        struct Iter
        {
            Iter(TFixedMap* map, size_t index) : Map(map), Index(index) {}

            TPair<TKey, TValue&> operator*() const
            {
                return { Map->Items[Index].first, Map->Items[Index].second };
            }

            Iter& operator++()
            {
                Index = Map->FindNextOccupiedSlot(Index + 1);
                return *this;
            }

            bool operator==(const Iter& other) const = default;

            TFixedMap* Map;
            size_t Index;
        };

        Iter begin() { return Iter(this, FindNextOccupiedSlot(0)); }
        Iter end() { return Iter(this, Capacity); }

        struct ConstIter
        {
            ConstIter(const TFixedMap* map, size_t index) : Map(map), Index(index) {}

            const TElement& operator*() const
            {
                return Map->Items[Index];
            }

            ConstIter& operator++()
            {
                Index = Map->FindNextOccupiedSlot(Index + 1);
                return *this;
            }

            bool operator==(const ConstIter& other) const = default;

            const TFixedMap* Map;
            size_t Index;
        };

        ConstIter begin() const { return ConstIter(this, FindNextOccupiedSlot(0)); }
        ConstIter end() const { return ConstIter(this, Capacity); }

    private:

        static size_t Hash(const TKey& key)
        {
            static THasher hasher;
            return hasher(key) % Capacity;
        }

        size_t FindSlot(const TKey& key) const
        {
            size_t hash = Hash(key);
            size_t index = hash & (Capacity - 1);
            size_t startIndex = index;
            while (Items[index].first != 0 && Items[index].first != key)
            {
                index = (index + 1) & (Capacity - 1);
                if (index == startIndex)
                    break;
            }
            return index;
        }

        size_t FindNextOccupiedSlot(size_t index) const
        {
            while (index < Capacity && Items[index].first == 0)
            {
                ++index;
            }
            return index;
        }

        TElement Items[N];
        size_t Size = 0;
    };
}
