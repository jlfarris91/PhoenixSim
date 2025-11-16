
#pragma once

#include "Color.h"
#include "FixedBlackboard.h"
#include "FixedPoint/FixedPoint.h"
#include "FixedPoint/FixedVector.h"

namespace Phoenix::Blackboard
{
    PHX_DECLARE_BLACKBOARD_TYPE(bool, EBlackboardValueTypes::Bool);
    PHX_DECLARE_BLACKBOARD_TYPE(uint32, EBlackboardValueTypes::UInt32);
    PHX_DECLARE_BLACKBOARD_TYPE(int32, EBlackboardValueTypes::Int32);
    PHX_DECLARE_BLACKBOARD_TYPE(FName, EBlackboardValueTypes::Name);
    PHX_DECLARE_BLACKBOARD_TYPE(Color, EBlackboardValueTypes::Color);

    PHX_DECLARE_BLACKBOARD_TYPE(Value, EBlackboardValueTypes::Value);
    PHX_DECLARE_BLACKBOARD_TYPE(InvValue, EBlackboardValueTypes::InvValue);
    // PHX_DECLARE_BLACKBOARD_TYPE(Distance, EBlackboardValueTypes::Distance); // Distance is the same type as Value
    PHX_DECLARE_BLACKBOARD_TYPE(Time, EBlackboardValueTypes::Time);
    PHX_DECLARE_BLACKBOARD_TYPE(Angle, EBlackboardValueTypes::Angle);
    PHX_DECLARE_BLACKBOARD_TYPE(Speed, EBlackboardValueTypes::Speed);
    PHX_DECLARE_BLACKBOARD_TYPE(Vec2, EBlackboardValueTypes::Vec2);

    template <>
    struct BlackboardValueConverter<bool>
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
    struct BlackboardValueConverter<TFixed<Tb, T>>
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

    template <class TBlackboard>
    struct BlackboardComplexValueAccessor<TBlackboard, Vec2>
    {
        static constexpr FName XName = "X"_n;
        static constexpr FName YName = "Y"_n;
        static constexpr blackboard_type_t VecType = static_cast<blackboard_type_t>(EBlackboardValueTypes::Vec2);

        static bool HasValue(const TBlackboard& set, blackboard_key_t key)
        {
            auto xKey = BlackboardKey::CombineKeyLo(key, static_cast<uint32>(XName));
            auto yKey = BlackboardKey::CombineKeyLo(key, static_cast<uint32>(YName));
            return set.template HasValue<Vec2::ComponentT>(xKey) &&
                   set.template HasValue<Vec2::ComponentT>(yKey);
        }

        static bool GetValue(const TBlackboard& set, blackboard_key_t key, Vec2& outValue)
        {
            auto xKey = BlackboardKey::CombineKeyLo(key, static_cast<uint32>(XName));
            auto yKey = BlackboardKey::CombineKeyLo(key, static_cast<uint32>(YName));
            return set.GetValue(xKey, outValue.X) && set.GetValue(yKey, outValue.Y);
        }

        static bool SetValue(TBlackboard& set, blackboard_key_t key, const Vec2& value)
        {
            auto xKey = BlackboardKey::CombineKeyLo(key, static_cast<uint32>(XName));
            auto yKey = BlackboardKey::CombineKeyLo(key, static_cast<uint32>(YName));
            return set.SetValue(xKey, value.X) && set.SetValue(yKey, value.Y);
        }

        static bool RemoveValue(TBlackboard& set, blackboard_key_t key)
        {
            auto xKey = BlackboardKey::CombineKeyLo(key, static_cast<uint32>(XName));
            auto yKey = BlackboardKey::CombineKeyLo(key, static_cast<uint32>(YName));
            return set.RemoveValue(xKey) && set.RemoveValue(yKey);
        }
    };

    template <class TBlackboard>
    struct BlackboardComplexValueAccessor<TBlackboard, Color>
    {
        static constexpr FName RName = "R"_n;
        static constexpr FName GName = "G"_n;
        static constexpr FName BName = "B"_n;
        static constexpr FName AName = "A"_n;
        static constexpr blackboard_type_t ColorType = static_cast<blackboard_type_t>(EBlackboardValueTypes::Color);

        static bool HasValue(const TBlackboard& set, const BlackboardKeyQuery& query)
        {
            auto colorQuery = query.WithType(ColorType);
            auto rQuery = colorQuery.CombineLo(static_cast<uint32>(RName));
            auto gQuery = colorQuery.CombineLo(static_cast<uint32>(GName));
            auto bQuery = colorQuery.CombineLo(static_cast<uint32>(BName));
            if (!set.template HasValue<uint8>(rQuery) ||
                !set.template HasValue<uint8>(gQuery) ||
                !set.template HasValue<uint8>(bQuery))
            {
                return false;
            }
            // Alpha is optional
            return true;
        }

        static bool GetValue(const TBlackboard& set, const BlackboardKeyQuery& query, Color& outValue)
        {
            auto colorQuery = query.WithType(ColorType);
            auto rQuery = colorQuery.CombineLo(static_cast<uint32>(RName));
            auto gQuery = colorQuery.CombineLo(static_cast<uint32>(GName));
            auto bQuery = colorQuery.CombineLo(static_cast<uint32>(BName));
            auto aQuery = colorQuery.CombineLo(static_cast<uint32>(AName));
            if (!set.template GetValue<uint8>(rQuery, outValue.R) ||
                !set.template GetValue<uint8>(gQuery, outValue.G) ||
                !set.template GetValue<uint8>(bQuery, outValue.B))
            {
                return false;
            }
            set.template GetValue<uint8>(aQuery, outValue.A);
            return true;
        }

        static bool SetValue(TBlackboard& set, blackboard_key_t key, const Color& value)
        {
            auto colorKey = BlackboardKey::Create(key, ColorType);
            auto rKey = BlackboardKey::CombineKeyLo(colorKey, static_cast<uint32>(RName));
            auto gKey = BlackboardKey::CombineKeyLo(colorKey, static_cast<uint32>(GName));
            auto bKey = BlackboardKey::CombineKeyLo(colorKey, static_cast<uint32>(BName));
            if (!set.template SetValue<uint8>(rKey, value.R) ||
                !set.template SetValue<uint8>(gKey, value.G) ||
                !set.template SetValue<uint8>(bKey, value.B))
            {
                return false;
            }
            // A is optional so only write it if it isn't the default value of 255
            if (value.A != 255)
            {
                auto aKey = BlackboardKey::CombineKeyLo(colorKey, static_cast<uint32>(AName));
                return set.template SetValue<uint8>(aKey, value.A);
            }
            return true;
        }

        static bool RemoveValue(TBlackboard& set, const BlackboardKeyQuery& query)
        {
            auto colorQuery = query.WithType(ColorType);
            auto rQuery = colorQuery.CombineLo(static_cast<uint32>(RName));
            auto gQuery = colorQuery.CombineLo(static_cast<uint32>(GName));
            auto bQuery = colorQuery.CombineLo(static_cast<uint32>(BName));
            auto aQuery = colorQuery.CombineLo(static_cast<uint32>(AName));
            if (!set.template RemoveValue<uint8>(rQuery) ||
                !set.template RemoveValue<uint8>(gQuery) ||
                !set.template RemoveValue<uint8>(bQuery))
            {
                return false;
            }
            // Alpha is optional
            set.template RemoveValue<uint8>(aQuery);
            return true;
        }
    };
}