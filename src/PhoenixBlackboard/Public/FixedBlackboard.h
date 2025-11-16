
#pragma once

#include <algorithm>
#include <execution>

#include "Name.h"
#include "Platform.h"
#include "Profiling.h"
#include "Containers/FixedArray.h"

namespace Phoenix::Blackboard
{
    using blackboard_key_t = uint64;
    using blackboard_value_t = int64;
    using blackboard_type_t = uint8;

    using BlackboardKVP = TPair<blackboard_key_t, blackboard_value_t>;

    static constexpr uint32 IgnoreKey = -1;
    static constexpr blackboard_type_t IgnoreType = -1;
    static constexpr blackboard_type_t UnknownType = 0;

    namespace BlackboardKey
    {
        // [FFFF'FF][FF][FFFF'FFFF]
        // [hi     ][ty][lo       ]
        static constexpr uint8 KeyHiShift       = 40;
        static constexpr uint8 KeyTypeShift     = 32;
        static constexpr uint64 KeyLoMask       = 0xFFFF'FFFFULL;
        static constexpr uint64 KeyHiMask       = 0xFF'FFFFULL << KeyHiShift;
        static constexpr uint64 KeyNoTypeMask   = KeyLoMask | KeyHiMask;
        static constexpr uint64 KeyTypeMask     = 0xFFULL << KeyTypeShift;

        PHX_FORCE_INLINE constexpr uint32 GetKeyLo(blackboard_key_t key)
        {
            return static_cast<uint32>(key & KeyLoMask);
        }

        PHX_FORCE_INLINE constexpr uint32 GetKeyHi(blackboard_key_t key)
        {
            return static_cast<uint32>((key & KeyHiMask) >> KeyHiShift);
        }

        PHX_FORCE_INLINE constexpr blackboard_key_t GetKeyNoType(blackboard_key_t key)
        {
            return key & KeyNoTypeMask;
        }

        PHX_FORCE_INLINE constexpr blackboard_type_t GetKeyType(blackboard_key_t key)
        {
            return static_cast<blackboard_type_t>((key & KeyTypeMask) >> KeyTypeShift);
        }

        PHX_FORCE_INLINE constexpr bool IsType(blackboard_key_t key, blackboard_type_t type)
        {
            return GetKeyType(key) == type;
        }

        PHX_FORCE_INLINE constexpr blackboard_key_t Create(uint32 lo, uint32 hi, blackboard_type_t type)
        {
            blackboard_key_t key = lo & KeyLoMask;
            key |= (static_cast<uint64>(hi) << KeyHiShift) & KeyHiMask;
            key |= (static_cast<uint64>(type) << KeyTypeShift) & KeyTypeMask;
            return key;
        }

        PHX_FORCE_INLINE constexpr blackboard_key_t Create(blackboard_key_t keyNoType, blackboard_type_t type)
        {
            blackboard_key_t keyWithType = keyNoType & KeyNoTypeMask;
            return keyWithType | ((static_cast<uint64>(type) << KeyTypeShift) & KeyTypeMask);
        }

        PHX_FORCE_INLINE constexpr blackboard_key_t CombineKeyLo(blackboard_key_t key, uint32 lo)
        {
            hash32_t newLo = Hashing::FN1VA32Combine(GetKeyLo(key), lo);
            return Create(newLo, GetKeyHi(key), GetKeyType(key));
        }

        PHX_FORCE_INLINE constexpr blackboard_key_t ClearKeyLo(blackboard_key_t key)
        {
            return Create(0, GetKeyHi(key), GetKeyType(key));
        }
    };

    static_assert(BlackboardKey::GetKeyLo(BlackboardKey::Create(123, 456, 16)) == 123);
    static_assert(BlackboardKey::GetKeyHi(BlackboardKey::Create(123, 456, 16)) == 456);
    static_assert(BlackboardKey::GetKeyType(BlackboardKey::Create(123, 456, 16)) == 16);
    static_assert(BlackboardKey::Create(123, 456, 16) == 0x1C8100000007B);

    template <uint32 N>
    class TFixedBlackboard;

    enum class EBlackboardValueTypes : blackboard_type_t
    {
        Unknown = UnknownType,

        // Standard PHX types
        Bool,
        UInt32,
        Int32,
        Name,
        Color,

        // Fixed Point types
        FIXED_POINT = 20,
        Value,
        InvValue,
        Distance,
        Time,
        Angle,
        Speed,
        Vec2,

        USER_DEFINED = 50,
    };

    // Fallback for unknown types
    template <class>
    struct BlackboardValueType
    {
        static constexpr blackboard_type_t Type = static_cast<blackboard_type_t>(EBlackboardValueTypes::Unknown);
    };

#define PHX_DECLARE_BLACKBOARD_TYPE(T, ValueType) \
    template <> \
    struct BlackboardValueType<T> \
    { \
        static constexpr blackboard_type_t Type = static_cast<blackboard_type_t>(ValueType); \
    }

    template <class T>
    struct BlackboardValueConverter
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

    template <class TBlackboardSet>
    struct BlackboardValues;

    struct BlackboardKeyQuery
    {
        BlackboardKeyQuery(blackboard_key_t key)
            : Filter(key)
        {
        }

        BlackboardKeyQuery(blackboard_key_t key, blackboard_type_t type)
            : Filter(BlackboardKey::Create(key, type))
        {
        }

        BlackboardKeyQuery(uint32 keyLo, uint32 keyHi, blackboard_type_t type)
            : Filter(BlackboardKey::Create(keyLo, keyHi, type))
        {
        }

        PHX_FORCE_INLINE BlackboardKeyQuery CombineLo(uint32 lo) const
        {
            return BlackboardKey::CombineKeyLo(Filter, lo);
        }

        PHX_FORCE_INLINE BlackboardKeyQuery WithType(blackboard_type_t type) const
        {
            return BlackboardKeyQuery(Filter, type);
        }

        bool operator()(const BlackboardKVP& item) const noexcept
        {
            if (BlackboardKey::GetKeyLo(item.first) == 0)
            {
                return false;
            }
            uint32 filterLo = BlackboardKey::GetKeyLo(Filter);
            uint32 itemLo = BlackboardKey::GetKeyLo(item.first);
            if (filterLo != IgnoreKey && filterLo != itemLo)
            {
                return false;
            }
            uint32 filterHi = BlackboardKey::GetKeyHi(Filter);
            uint32 itemHi = BlackboardKey::GetKeyHi(item.first);
            if (filterHi != IgnoreKey && filterHi != itemHi)
            {
                return false;
            }
            uint8 filterType = BlackboardKey::GetKeyType(Filter);
            uint8 itemType = BlackboardKey::GetKeyType(item.first);
            if (filterType != IgnoreType && filterType != itemType)
            {
                return false;
            }
            return true;
        }

        blackboard_key_t Filter;
    };

    template <class, class>
    struct BlackboardComplexValueAccessor{};

    template <class TBlackboard, class TValue>
    concept BlackboardComplexValueAccessor_HasValue = requires(const TBlackboard& set, const BlackboardKeyQuery& query)
    {
        { BlackboardComplexValueAccessor<TBlackboard, TValue>::HasValue(set, query) } -> std::same_as<bool>;
    };

    template <class TBlackboard, class TValue>
    concept BlackboardComplexValueAccessor_SetValue = requires(TBlackboard& set, blackboard_key_t key, const TValue& value)
    {
        { BlackboardComplexValueAccessor<TBlackboard, TValue>::SetValue(set, key, value) } -> std::same_as<bool>;
    };

    template <class TBlackboard, class TValue>
    concept BlackboardComplexValueAccessor_GetValue = requires(const TBlackboard& set, const BlackboardKeyQuery& query, TValue& outValue)
    {
        { BlackboardComplexValueAccessor<TBlackboard, TValue>::GetValue(set, query, outValue) } -> std::same_as<bool>;
    };

    template <class TBlackboard, class TValue>
    concept BlackboardComplexValueAccessor_RemoveValue = requires(TBlackboard& set, const BlackboardKeyQuery& query)
    {
        { BlackboardComplexValueAccessor<TBlackboard, TValue>::RemoveValue(set, query) } -> std::same_as<bool>;
    };

    template <uint32 N>
    class TFixedBlackboard
    {
    public:

        uint32 GetSize() const
        {
            return Items.Num();
        }

        uint32 GetNumActive() const
        {
            return NumActive;
        }

        // Returns true if the blackboard has a value for the given key query.
        bool HasValue(const BlackboardKeyQuery& query) const
        {
            uint32 index = IndexOfKey(query);
            return Items.IsValidIndex(index);
        }

        // Returns true if the blackboard has a value for the given key query.
        template <class T>
        bool HasValue(const BlackboardKeyQuery& query) const requires(BlackboardComplexValueAccessor_HasValue<TFixedBlackboard, T>)
        {
            return BlackboardComplexValueAccessor<TFixedBlackboard, T>::HasValue(*this, query);
        }

        // Returns true if the blackboard has a value for the given key that is the expected type.
        template <class T>
        bool HasValue(blackboard_key_t key, blackboard_type_t expectedType = BlackboardValueType<T>::Type) const
        {
            return HasValue<T>(BlackboardKeyQuery(key, expectedType));
        }

        bool SetValue(blackboard_key_t key, blackboard_value_t value)
        {
            return InsertKVP(key, value);
        }

        template <class T>
        bool SetValue(blackboard_key_t key, const T& value, blackboard_type_t type)
        {
            PHX_PROFILE_ZONE_SCOPED;

            blackboard_key_t keyWithType = BlackboardKey::Create(key, type);
            if constexpr (BlackboardComplexValueAccessor_SetValue<TFixedBlackboard, T>)
            {
                return BlackboardComplexValueAccessor<TFixedBlackboard, T>::SetValue(*this, keyWithType, value);
            }
            else
            {
                return SetValue(keyWithType, BlackboardValueConverter<T>::ConvertTo(value));
            }
        }

        template <class T>
        bool SetValue(blackboard_key_t key, const T& value)
        {
            PHX_PROFILE_ZONE_SCOPED;

            if constexpr (BlackboardComplexValueAccessor_SetValue<TFixedBlackboard, T>)
            {
                return BlackboardComplexValueAccessor<TFixedBlackboard, T>::SetValue(*this, key, value);
            }
            else
            {
                return SetValue(key, BlackboardValueConverter<T>::ConvertTo(value));
            }
        }

        bool GetValue(const BlackboardKeyQuery& query, blackboard_value_t& outValue) const
        {
            PHX_PROFILE_ZONE_SCOPED;

            uint32 index = IndexOfKey(query);
            if (!Items.IsValidIndex(index))
            {
                return false;
            }
            outValue = Items[index].second;
            return true;
        }

        bool GetValue(blackboard_key_t key, blackboard_value_t& outValue) const
        {
            return GetValue(BlackboardKeyQuery(key), outValue);
        }

        template <class T>
        bool GetValue(const BlackboardKeyQuery& query, T& outValue) const
        {
            PHX_PROFILE_ZONE_SCOPED;

            if constexpr (BlackboardComplexValueAccessor_GetValue<TFixedBlackboard, T>)
            {
                return BlackboardComplexValueAccessor<TFixedBlackboard, T>::GetValue(*this, query, outValue);
            }
            else
            {
                blackboard_value_t value;
                if (!GetValue(query, value))
                {
                    return false;
                }
                outValue = BlackboardValueConverter<T>::ConvertFrom(value);
                return true;
            }
        }

        template <class T>
        bool GetValue(blackboard_key_t key, T& outValue) const
        {
            BlackboardKeyQuery query(key, BlackboardValueType<T>::Type);
            return GetValue<T>(query, outValue);
        }

        template <class T = blackboard_value_t>
        bool RemoveValue(const BlackboardKeyQuery& query)
        {
            PHX_PROFILE_ZONE_SCOPED;

            if constexpr (BlackboardComplexValueAccessor_RemoveValue<TFixedBlackboard, T>)
            {
                return BlackboardComplexValueAccessor<TFixedBlackboard, T>::RemoveValue(*this, query);
            }
            else
            {
                uint32 index = IndexOfKey(query);
                if (!Items.IsValidIndex(index))
                {
                    return false;
                }
                Items[index].first = BlackboardKey::ClearKeyLo(Items[index].first);
                --NumActive;
                return true;
            }
        }

        template <class T = blackboard_value_t>
        bool RemoveValue(blackboard_key_t key, blackboard_type_t expectedType = BlackboardValueType<T>::Type)
        {
            return RemoveValue<T>(BlackboardKeyQuery(key, expectedType));
        }

        uint32 RemoveAll(const BlackboardKeyQuery& query)
        {
            PHX_PROFILE_ZONE_SCOPED;

            uint32 numRemoved = 0;

            uint32 index = IndexOfKey(query);
            while (Items.IsValidIndex(index))
            {
                Items[index].first = BlackboardKey::ClearKeyLo(Items[index].first);
                ++numRemoved;
                --NumActive;

                uint32 nextIndex = IndexOfKey(query, index + 1);
                if (!Items.IsValidIndex(nextIndex) && numRemoved < 3)
                {
                    __debugbreak();
                }

                index = nextIndex;
            }

            return numRemoved;
        }

        void SortAndCompact()
        {
            PHX_PROFILE_ZONE_SCOPED;

            struct KVPSort
            {
                bool operator()(const BlackboardKVP& a, const BlackboardKVP& b) const
                {
                    if (BlackboardKey::GetKeyLo(a.first) == 0)
                        return false;
                    if (BlackboardKey::GetKeyLo(b.first) == 0)
                        return true;
                    return a.first < b.first;
                }
            } static sort;
        
            std::sort(
                std::execution::par,
                Items.begin(),
                Items.end(),
                sort);

            uint32 i = 0;
            for (; i < Items.Num(); ++i)
            {
                if (BlackboardKey::GetKeyLo(Items[i].first) == 0)
                {
                    break;
                }
            }

            SortedCount = i;
            NumActive = i;
            Items.SetSize(SortedCount);
        }

        BlackboardValues<TFixedBlackboard> Enumerate(uint32 keyHi) const
        {
            return BlackboardValues<TFixedBlackboard>(this, keyHi);
        }

    private:

        struct CompareKeyHi
        {
            bool operator()(const BlackboardKVP& item, uint32 keyHi) const noexcept
            {
                return BlackboardKey::GetKeyHi(item.first) < keyHi;
            }
        };

        uint32 IndexOfKey(const BlackboardKeyQuery& query, uint32 startIndex = 0) const
        {
            PHX_PROFILE_ZONE_SCOPED;
            
            static CompareKeyHi compare;

            // Search the sorted section first
            if (startIndex < SortedCount)
            {
                uint32 queryKeyHi = BlackboardKey::GetKeyHi(query.Filter);
                
                // The range is a subset of Items that has already been sorted.
                auto begin = Items.begin() + startIndex;
                auto end = Items.begin() + SortedCount;

                // Find the range of contiguous sorted keys that have the same hi value
                auto lower = std::lower_bound(begin, end, queryKeyHi, compare);

                // We found at least one key in that range
                for (; lower != end; ++lower)
                {
                    if (query(*lower))
                    {
                        return static_cast<uint32>(lower - Items.begin());
                    }

                    // We've reached the end of the range
                    uint32 lo = BlackboardKey::GetKeyLo(lower->first);
                    uint32 hi = BlackboardKey::GetKeyHi(lower->first);
                    if (lo != 0 && hi != queryKeyHi)
                        break;
                }
            }

            // Then search the unsorted section
            uint32 i = SortedCount;
            while (i < Items.Num())
            {
                if (query(Items[i]))
                {
                    return i;
                }
            }

            return Index<uint32>::None;
        }

        bool InsertKVP(blackboard_key_t key, blackboard_value_t value)
        {
            PHX_PROFILE_ZONE_SCOPED;

            if (Items.IsFull())
            {
                return false;
            }

            Items.EmplaceBack(key, value);
            ++NumActive;
            return true;
        }

        template <class TBlackboardSet>
        friend struct BlackboardValues;

        TFixedArray<BlackboardKVP, N> Items;
        uint32 SortedCount = 0;
        uint32 NumActive = 0;
    };

    template <class TBlackboardSet>
    struct BlackboardValues
    {
        using TKeyQuery = typename TBlackboardSet::BlackboardKeyQuery;

        BlackboardValues() = default;

        BlackboardValues(const TBlackboardSet* set, const TKeyQuery& query)
            : Owner(set)
            , Query(query)
        {
        }

        struct KeyHiIter
        {
            KeyHiIter(const TBlackboardSet* owner, const TKeyQuery& query, uint32 index)
                : Owner(owner)
                , Query(query)
                , Index(index)
            {
                if (Owner)
                {
                    Index = Owner->IndexOfKey(Query, Index);
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
                    Index = Owner->IndexOfKey(Query, Index + 1);
                }
                return *this;
            }

            bool operator==(const KeyHiIter& other) const = default;

            const TBlackboardSet* Owner;
            TKeyQuery Query;
            uint32 Index;
        };

        KeyHiIter begin() const
        {
            uint32 index = Owner ? Owner->IndexOfKey(Query) : 0;
            return KeyHiIter(Owner, Query, index);
        }

        KeyHiIter end() const
        {
            uint32 index = Owner ? Owner->Items.Num() : 0;
            return KeyHiIter(Owner, Query, index);
        }

    private:

        const TBlackboardSet* Owner;
        TKeyQuery Query;
    };
}

#include "FixedBlackboard.inl"