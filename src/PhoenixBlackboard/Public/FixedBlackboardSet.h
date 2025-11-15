
#pragma once

#include <algorithm>
#include <execution>

#include "Platform.h"
#include "Containers/FixedArray.h"
#include "FixedPoint/FixedPoint.h"

namespace Phoenix
{
    using blackboard_key_t = uint64;
    using blackboard_value_t = int64;

    template <class T>
    struct BlackboardKeyConverter
    {
        static T ConvertFrom(blackboard_value_t value)
        {
            return static_cast<T>(value);
        }
        static blackboard_value_t ConvertTo(T value)
        {
            return static_cast<blackboard_value_t>(value);
        }
    };

    template <>
    struct BlackboardKeyConverter<bool>
    {
        static bool ConvertFrom(blackboard_value_t value)
        {
            return value != 0;
        }
        static blackboard_value_t ConvertTo(bool value)
        {
            return value;
        }
    };

    template <uint8 Tb, class T>
    struct BlackboardKeyConverter<TFixed<Tb, T>>
    {
        static TFixed<Tb, T> ConvertFrom(blackboard_value_t value)
        {
            return reinterpret_cast<TFixedQ_T<T>>(value);
        }
        static blackboard_value_t ConvertTo(const TFixed<Tb, T>& value)
        {
            return reinterpret_cast<blackboard_value_t>(value.Value);
        }
    };

    template <class TBlackboardSet>
    struct BlackboardSubKeys;

    template <uint32 N>
    class TFixedBlackboardSet
    {
    public:
        using TItem = TPair<blackboard_key_t, blackboard_value_t>;

        bool HasKey(blackboard_key_t key) const
        {
            return Items.IsValidIndex(FindSlot(key));
        }

        bool Set(blackboard_key_t key, blackboard_value_t value)
        {
            return InsertKVP(key, value);
        }

        template <class T, std::enable_if_t<!std::is_same_v<T, blackboard_value_t>>>
        bool Set(blackboard_key_t key, T value) const
        {
            return Set(key, BlackboardKeyConverter<T>::ConvertTo(value));
        }

        blackboard_value_t Get(blackboard_key_t key) const
        {
            uint32 index = FindSlot(key);
            if (!Items.IsValidIndex(index))
            {
                return {};
            }
            return Items[index].second;
        }

        template <class T, std::enable_if_t<!std::is_same_v<T, blackboard_value_t>>>
        T Get(blackboard_key_t key) const
        {
            return BlackboardKeyConverter<T>::ConvertFrom(Get(key));
        }

        bool TryGet(blackboard_key_t key, blackboard_value_t& outValue) const
        {
            uint32 index = FindSlot(key);
            if (!Items.IsValidIndex(index))
            {
                return false;
            }
            outValue = Items[index].second;
            return true;
        }

        template <class T>
        bool TryGet(blackboard_key_t key, T& outValue) const
        {
            blackboard_value_t value;
            if (!TryGet(key, value))
            {
                return false;
            }
            outValue = BlackboardKeyConverter<T>::ConvertFrom(value);
            return true;
        }

        bool RemoveKey(blackboard_key_t key)
        {
            uint32 index = FindSlot(key);
            if (!Items.IsValidIndex(index))
            {
                return false;
            }
            Items[index] = {};
            return true;
        }

        uint32 RemoveKeys(uint32 keyHi)
        {
            uint32 numRemoved = 0;

            uint32 index = FindNextSlotWithKeyHi(0, keyHi);
            while (Items.IsValidIndex(index))
            {
                Items[index] = {};
                ++numRemoved;

                index = FindNextSlotWithKeyHi(index + 1, keyHi);
            }

            return numRemoved;
        }

        void SortAndCompact()
        {
            struct Sorter
            {
                size_t operator<(const TItem& a, const TItem& b) const
                {
                    if (a.first == 0)
                        return false;
                    if (b.first == 0)
                        return true;
                    return a.first < b.second;
                }
            };
            
            std::ranges::sort(Items, Sorter(), &TItem::first);

            auto lb = std::ranges::lower_bound(Items, 0, {}, &TItem::first);
            SortedCount = lb - Items.begin();

            Items.SetSize(SortedCount);
        }

        BlackboardSubKeys<TFixedBlackboardSet> Enumerate(uint32 keyHi) const
        {
            return BlackboardSubKeys<TFixedBlackboardSet>(this, keyHi);
        }

    private:

        static constexpr uint32 GetKeyHi(blackboard_key_t key)
        {
            return static_cast<uint32>(key >> 32) & 0xFFFFFFFF;
        }

        uint32 FindSlot(blackboard_key_t key) const
        {            
            auto iter = std::ranges::find(Items.begin(), Items.begin() + SortedCount, key, &TItem::first);
            if (iter != Items.end())
            {
                return static_cast<uint32>(iter - Items.begin());
            }

            uint32 i = SortedCount;
            while (i < Items.Num())
            {
                if (Items[i].first == key)
                {
                    return i;
                }
            }

            return Index<uint32>::None;
        }

        uint32 FindNextSlotWithKeyHi(uint32 index, uint32 keyHi) const
        {
            while (index < SortedCount)
            {
                if (GetKeyHi(Items[index].first) == keyHi)
                {
                    return index;
                }
                ++index;
            }

            while (index < Items.Num())
            {
                if (GetKeyHi(Items[index].first) == keyHi)
                {
                    return index;
                }
                ++index;
            }

            return index;
        }

        bool InsertKVP(blackboard_key_t key, blackboard_value_t value)
        {
            if (Items.IsFull())
            {
                return false;
            }

            Items.EmplaceBack(key, value);
            return true;
        }

        template <class TBlackboardSet>
        friend struct BlackboardSubKeys;

        TFixedArray<TItem, N> Items;
        uint32 SortedCount = 0;
    };

    template <class TBlackboardSet>
    struct BlackboardSubKeys
    {
        BlackboardSubKeys() = default;

        BlackboardSubKeys(const TBlackboardSet* set, uint32 keyHi)
            : Owner(set)
            , KeyHi(keyHi)
        {
        }

        struct KeyHiIter
        {
            KeyHiIter(const TBlackboardSet* owner, uint32 keyHi, uint32 index)
                : Owner(owner)
                , KeyHi(keyHi)
                , Index(index)
            {
                if (Owner)
                {
                    Index = Owner->FindNextSlotWithKeyHi(Index, KeyHi);
                }
            }

            const typename TBlackboardSet::TItem& operator*() const
            {
                static typename TBlackboardSet::TItem null;
                return Owner ? Owner->Items[Index] : null;
            }

            KeyHiIter& operator++()
            {
                if (Owner)
                {
                    Index = Owner->FindNextSlotWithKeyHi(Index + 1, KeyHi);
                }
                return *this;
            }

            bool operator==(const KeyHiIter& other) const = default;

            const TBlackboardSet* Owner;
            uint32 KeyHi;
            uint32 Index;
        };

        KeyHiIter begin() const
        {
            uint32 index = Owner ? Owner->FindNextSlotWithKeyHi(KeyHi) : 0;
            return KeyHiIter(Owner, KeyHi, index);
        }

        KeyHiIter end() const
        {
            uint32 index = Owner ? Owner->Items.Num() : 0;
            return KeyHiIter(Owner, KeyHi, index);
        }

    private:

        const TBlackboardSet* Owner;
        uint32 KeyHi;
    };
}
