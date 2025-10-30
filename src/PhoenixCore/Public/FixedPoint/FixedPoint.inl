
#pragma once

#include "Platform.h"
#include <type_traits>  // For std::conditional and std::is_same

namespace Phoenix
{
    ///////////////////////////////////////////////////////////////////////////
    // 
    // Operations
    //
    ///////////////////////////////////////////////////////////////////////////

    //
    // Equality
    //

    // operator==

    // TFixed<Tb, T> == TFixed<Tb, T>
    template<uint8 Tb, class T>
    constexpr bool Equals(const TFixed<Tb, T>& lhs, const TFixed<Tb, T>& rhs, const TFixed<Tb, T>& threshold)
    {
        auto v = int64(lhs.Value) - rhs.Value; 
        return (v < 0 ? -v : v) <= threshold.Value;
    }

    // TFixed<Tb, T> == TFixed<Tb, T>
    template<uint8 Tb, class T>
    constexpr bool operator==(const TFixed<Tb, T>& lhs, const TFixed<Tb, T>& rhs)
    {
        return Equals(lhs, rhs, TFixed<Tb, T>(TFixedQ_T<T>(1)));
    }

    // TFixed<Tb, T> == TFixed<Ub, U>
    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr bool operator==(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        auto a = int64(lhs.Value) * (1 << Ub);
        auto b = int64(rhs.Value) * (1 << Tb);
        auto v = a - b; 
        return (v < 0 ? -v : v) <= 1;
    }

    // TFixed == double
    template<uint8 Tb, class T>
    constexpr bool operator==(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs.Value == TFixed<Tb, T>::ConvertToQ(rhs);
    }

    // double == TFixed 
    template<uint8 Tb, class T>
    constexpr bool operator==(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) == rhs;
    }

    // TInvFixed == TInvFixed 
    template<uint8 Tb, class T>
    constexpr bool operator==(const TInvFixed<Tb, T>& lhs, const TInvFixed<Tb, T>& rhs)
    {
        return lhs.Value == rhs.Value;
    }

    // TInvFixed == double 
    template<uint8 Tb, class T>
    constexpr bool operator==(const TInvFixed<Tb, T>& lhs, double rhs)
    {
        return lhs.Value == TInvFixed<Tb, T>::ToFixedValue(rhs);
    }

    // double == TInvFixed
    template<uint8 Tb, class T>
    constexpr bool operator==(double lhs, const TInvFixed<Tb, T>& rhs)
    {
        return TInvFixed<Tb, T>::ToFixedValue(lhs) == rhs;
    }

    // operator!=

    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr bool operator!=(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        return !operator==(lhs, rhs);
    }

    template<uint8 Tb, class T>
    constexpr bool operator!=(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs.Value != TFixed<Tb, T>::ConvertToQ(rhs);
    }

    template<uint8 Tb, class T>
    constexpr bool operator!=(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) != rhs;
    }

    // operator<

    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr bool operator<(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
            return int64(lhs.Value) * Ud / Td < rhs.Value;
        else
            return lhs.Value < int64(rhs.Value) * Td / Ud;
    }

    template<uint8 Tb, class T>
    constexpr bool operator<(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs < TFixed<Tb, T>(rhs);
    }

    template<uint8 Tb, class T>
    constexpr bool operator<(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) < rhs;
    }

    // operator<=

    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr bool operator<=(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
            return int64(lhs.Value) * Ud / Td <= rhs.Value;
        else
            return lhs.Value <= int64(rhs.Value) * Td / Ud;
    }

    template<uint8 Tb, class T>
    constexpr bool operator<=(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs <= TFixed<Tb, T>(rhs);
    }

    template<uint8 Tb, class T>
    constexpr bool operator<=(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) <= rhs;
    }

    // operator>

    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr bool operator>(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
            return int64(lhs.Value) * Ud / Td > rhs.Value;
        else
            return lhs.Value > int64(rhs.Value) * Td / Ud;
    }

    template<uint8 Tb, class T>
    constexpr bool operator>(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs > TFixed<Tb, T>(rhs);
    }

    template<uint8 Tb, class T>
    constexpr bool operator>(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) > rhs;
    }

    // operator>=

    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr bool operator>=(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
            return int64(lhs.Value) * Ud / Td >= rhs.Value;
        else
            return lhs.Value >= int64(rhs.Value) * Td / Ud;
    }

    template<uint8 Tb, class T>
    constexpr bool operator>=(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs >= TFixed<Tb, T>(rhs);
    }

    template<uint8 Tb, class T>
    constexpr bool operator>=(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) >= rhs;
    }

    //
    // Addition
    //

    // template <uint8 Tb, class T, int64 Ub, class U>
    // constexpr auto ChkAdd(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    // {
    //     constexpr int64 Td = 1 << Tb;
    //     constexpr int64 Ud = 1 << Ub;
    //     if constexpr (Tb < Ub)
    //     {
    //         using CR = ChkResult<TFixed<Ub, TCommonQ_T<T, U>>>;
    //         auto v = ChkMult_<T>(int64(lhs.Value), Ud / Td);
    //         if (v.bOverflowed)
    //         {
    //             auto v128 = FixedUtils::Mult128(int64(lhs.Value), Ud / Td);
    //             v.Value = v128.template NarrowToI64<Ub>();
    //         }
    //         auto v2 = ChkAdd_<T>(int64(rhs.Value), v.Value);
    //         if (v2.bOverflowed)
    //         {
    //             return CR();
    //         }
    //         return CR(TFixed<Ub, TCommonQ_T<T, U>>(Q64(v2.Value)));
    //     }
    //     else
    //     {
    //         using CR = ChkResult<TFixed<Tb, TCommonQ_T<T, U>>>;
    //         auto v = ChkMult_<T>(int64(rhs.Value), Td / Ud);
    //         if (v.bOverflowed)
    //         {
    //             auto v128 = FixedUtils::Mult128(int64(rhs.Value), Td / Ud);
    //             v.Value = v128.template NarrowToI64<Ub>();
    //         }
    //         auto v2 = ChkAdd_<T>(int64(lhs.Value), v.Value);
    //         if (v2.bOverflowed)
    //         {
    //             return CR();
    //         }
    //         return CR(TFixed<Tb, TCommonQ_T<T, U>>(Q64(v2.Value)));
    //     }
    // }

    // TFixed<Tb, T> + TFixed<Ub, U>
    template <uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto operator+(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
        {
            return TFixed<Ub, int64>(Q64(int64(lhs.Value) * Ud / Td + rhs.Value));
        }
        else
        {
            return TFixed<Tb, int64>(Q64(lhs.Value + int64(rhs.Value) * Td / Ud));
        }
    }

    // TFixed + double
    template <uint8 Tb, class T>
    constexpr auto operator+(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs + TFixed<Tb, T>(rhs);
    }

    // double + TFixed
    template <uint8 Tb, class T>
    constexpr auto operator+(double lhs, const TFixed<Tb, T>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs + lhs;
    }

    // TFixed<Tb, T> += TFixed<Ub, U>
    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto& operator+=(TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        lhs = lhs + rhs;
        return lhs;
    }

    // TFixed += double
    template<uint8 Tb, class T>
    constexpr auto& operator+=(TFixed<Tb, T>& lhs, double rhs)
    {
        lhs += TFixed<Tb, T>(rhs);
        return lhs;
    }

    // TInvFixed + TInvFixed
    template <uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto operator+(const TInvFixed<Tb, T>& lhs, const TInvFixed<Ub, U>& rhs)
    {
        constexpr auto Vb = Tb > Ub ? Tb : Ub;
        constexpr auto Vd = Tb > Ub ? Tb : Ub;
        using V = TCommonQ_T<T, U>;
        int64 n = (int64(rhs.Value) << (Vb - Ub)) + (lhs.Value << (Vb - Tb));
        if (n == 0)
        {
            return TInvFixed<Vd, V>{};
        }
        int64 d = (int64(rhs.Value) << (Vb - Ub)) * (lhs.Value << (Vb - Tb));
        return TInvFixed<Vd, V>(TFixedQ_T<V>(d / n));
    }

    // TInvFixed + double
    template <uint8 Tb, class T>
    constexpr auto operator+(const TInvFixed<Tb, T>& lhs, double rhs)
    {
        return TInvFixed<Tb, T>(TFixedQ_T<T>(lhs.Value + TFixed<Tb, T>::ToFixedValue(rhs)));
    }

    // double + TInvFixed
    template <uint8 Tb, class T>
    constexpr auto operator+(double lhs, const TInvFixed<Tb, T>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs + lhs;
    }

    // TInvFixed += TInvFixed
    template <uint8 Tb, class T>
    constexpr auto& operator+=(TInvFixed<Tb, T>& lhs, const TInvFixed<Tb, T>& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    // TInvFixed += double
    template <uint8 Tb, class T>
    constexpr auto& operator+=(TInvFixed<Tb, T>& lhs, double rhs)
    {
        lhs += TFixed<Tb, T>(rhs);
        return lhs;
    }

    //
    // Subtraction
    //

    // TFixed<Tb, T> - TFixed<Ub, U>
    template <uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto operator-(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        return lhs + -rhs;
    }

    // TFixed - double
    template <uint8 Tb, class T>
    constexpr auto operator-(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs - TFixed<Tb, T>(rhs);
    }

    // double - TFixed
    template <uint8 Tb, class T>
    constexpr auto operator-(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) - rhs;
    }

    // -TFixed
    template <uint8 Tb, class T>
    constexpr auto operator-(const TFixed<Tb, T>& lhs)
    {
        return TFixed<Tb, T>(TFixedQ_T<T>(-lhs.Value));
    }

    // TFixed<Tb, T> -= TFixed<Ub, U>
    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto& operator-=(TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        lhs = lhs - rhs;
        return lhs;
    }

    // TFixed -= double
    template<uint8 Tb, class T>
    constexpr auto& operator-=(TFixed<Tb, T>& lhs, double rhs)
    {
        lhs -= TFixed<Tb, T>(rhs);
        return lhs;
    }

    // TInvFixed - TInvFixed
    template <uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto operator-(const TInvFixed<Tb, T>& lhs, const TInvFixed<Ub, U>& rhs)
    {
        constexpr auto Vb = Tb > Ub ? Tb : Ub;
        constexpr auto Vd = Tb > Ub ? Tb : Ub;
        using V = int64;
        V n = (V(rhs.Value) << (Vb - Ub)) - (lhs.Value << (Vb - Tb));
        if (n == 0)
        {
            return TInvFixed<Vd, V>{};
        }
        V d = (V(rhs.Value) << (Vb - Ub)) * (lhs.Value << (Vb - Tb));
        return TInvFixed<Vd, V>(TFixedQ_T<V>(d / n));
    }

    // TInvFixed - double
    template<uint8 Tb, class T>
    constexpr auto operator-(const TInvFixed<Tb, T>& lhs, double rhs)
    {
        return lhs - TFixed<Tb, T>(rhs);
    }

    // double - TInvFixed
    template<uint8 Tb, class T>
    constexpr auto operator-(double lhs, TInvFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) - rhs;
    }

    // TInvFixed -= TInvFixed
    template <uint8 Tb, class T>
    constexpr auto& operator-=(TInvFixed<Tb, T>& lhs, const TInvFixed<Tb, T>& rhs)
    {
        lhs = lhs - rhs;
        return lhs;
    }

    // TInvFixed -= double
    template<uint8 Tb, class T>
    constexpr auto& operator-=(TInvFixed<Tb, T>& lhs, double rhs)
    {
        lhs -= TInvFixed<Tb, T>(TFixedQ_T<T>(TFixed<Tb, T>::ToFixedValue(rhs)));
        return lhs;
    }

    //
    // Multiplication
    //

    // template <uint8 Tb, class T, int64 Ub, class U>
    // constexpr auto ChkMult(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    // {
    //     constexpr auto MIN = Tb < Ub ? Tb : Ub;
    //     constexpr auto MAX = Tb > Ub ? Tb : Ub;
    //     using CR = ChkResult<TFixed<MAX, int64>>;
    //     auto v = ChkMult_<int64, int64>(lhs.Value, rhs.Value);
    //     if (v.bOverflowed)
    //     {
    //         auto v128 = FixedUtils::Mult128(int64(lhs.Value), rhs.Value);
    //         v.Value = v128.template NarrowToI64<Ub>();
    //     }
    //     return CR(TFixed<MAX, int64>(Q64(v.Value >> MIN)));
    // }

    // TFixed<Tb, T> * TFixed<Ub, U>
    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto operator*(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr auto MIN = Tb < Ub ? Tb : Ub;
        constexpr auto MAX = Tb > Ub ? Tb : Ub;
        return TFixed<MAX, int64>(Q64((int64(lhs.Value) * rhs.Value) >> MIN));
    }

    // TFixed * double
    template<uint8 Tb, class T>
    constexpr auto operator*(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs * TFixed<Tb, T>(rhs);
    }

    // double * TFixed
    template <uint8 Tb, class T>
    constexpr auto operator*(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) * rhs;
    }

    // TFixed<Tb, T> *= TFixed<Ub, U>
    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto& operator*=(TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }

    // TFixed *= double
    template<uint8 Tb, class T>
    constexpr auto& operator*=(TFixed<Tb, T>& lhs, double rhs)
    {
        lhs *= TFixed<Tb, T>(rhs);
        return lhs;
    }

    // TFixed<Tb, T> * TInvFixed<Ub, U>
    // X*(1/Y) is just X/Y
    template <uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto operator*(const TFixed<Tb, T>& lhs, const TInvFixed<Ub, U>& rhs)
    {
        return lhs / TFixed<Ub, U>(TFixedQ_T<U>(rhs.Value));
    }

    // TInvFixed<Tb, T> * TFixed<Ub, U>
    // (1/X)*Y is just Y/X
    template <uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto operator*(const TInvFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs * lhs;
    }

    // TInvFixed * TInvFixed
    // (1/X)*(1/Y) is just 1/(X*Y)
    template <uint8 Tb, class T>
    constexpr auto operator*(const TInvFixed<Tb, T>& lhs, const TInvFixed<Tb, T>& rhs)
    {
        return TInvFixed<Tb, T>(Q64(int64(lhs.Value) * (1 << Tb) * rhs.Value));
    }

    //
    // Division
    //

    // TFixed<Tb, T> / TFixed<Ub, U>
    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto operator/(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        // This is the output type. For now we'll just return the same type as the lhs.
        using V = T;
        constexpr uint8 Vb = Tb;

        constexpr int32 shift = Ub + Vb - Tb; 
        int64 v = lhs.Value;

        if constexpr (shift >= 0)
        {
            v <<= shift;
        }
        else
        {
            v >>= -shift;
        }

        v /= static_cast<int64>(rhs.Value);

        return TFixed<Vb, V>(TFixedQ_T<V>(v));
    }

    // TFixed / double
    template <uint8 Tb, class T>
    constexpr auto operator/(const TFixed<Tb, T>& lhs, double rhs)
    {
        return TFixed<Tb, T>(Q64(lhs.Value / rhs));
    }

    // double / TFixed
    template <uint8 Tb, class T>
    constexpr auto operator/(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) / rhs;
    }

    // TFixed<Tb, T> /= TFixed<Ub, U>
    template<uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto& operator/=(TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        lhs = lhs / rhs;
        return lhs;
    }

    // TFixed /= double
    template<uint8 Tb, class T>
    constexpr auto& operator/=(TFixed<Tb, T>& lhs, double rhs)
    {
        lhs = lhs / TFixed<Tb, T>(rhs);
        return lhs;
    }

    // TFixed<Tb, T> / TInvFixed<Ub, U>
    // X/(1/Y) is just X*Y
    template <uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto operator/(const TFixed<Tb, T>& lhs, const TInvFixed<Ub, U>& rhs)
    {
        return lhs * TFixed<Ub, U>(TFixedQ_T<U>(rhs.Value));
    }

    // TInvFixed<Tb, T> / TFixed<Ub, U>
    // (1/X)/Y is just 1/(X*Y)
    template <uint8 Tb, class T, uint8 Ub, class U>
    constexpr auto operator/(const TInvFixed<Tb, T>& lhs, const TInvFixed<Ub, U>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs * lhs;
    }

    ///////////////////////////////////////////////////////////////////////////
    // 
    // Utility
    //
    ///////////////////////////////////////////////////////////////////////////

    template <uint8 Tb, class T>
    constexpr static TFixed<Tb, T> Abs(const TFixed<Tb, T>& value)
    {
        return TFixedQ_T<T>(Abs_(value.Value));
    }

    template <class T>
    constexpr TInvFixed<T::B, typename T::ValueT> OneDivBy(const T& v)
    {
        return TFixedQ_T<typename T::ValueT>(v.Value);
    }

    template <uint8 Tb, class T>
    constexpr TFixed<Tb, T> OneDivBy(const TInvFixed<Tb, T>& v)
    {
        return TFixedQ_T<T>(v.Value);
    }
}