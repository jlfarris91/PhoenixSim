#pragma once

#include <limits>

#include "FixedUtils.h"
#include "PlatformTypes.h"

#ifndef TFIXED_DEBUG
#if DEBUG
#define TFIXED_DEBUG 1
#else 
#define TFIXED_DEBUG 0
#endif
#endif

#if TFIXED_DEBUG
#define TFIXED_DEBUG_FIELD(x) , _Debug((x))
#else
#define TFIXED_DEBUG_FIELD(x)
#endif

namespace Phoenix
{
    template <class T>
    constexpr static T Abs_(T value)
    {
        // constexpr T cBits = (sizeof(T) << 3) - 1;
        // return (value ^ (value >> cBits)) - (value >> cBits);
        return value < 0 ? -value : value;
    }

    template <class T>
    struct ChkResult
    {
        constexpr ChkResult() : bOverflowed(true) {}
        constexpr ChkResult(T v) : bOverflowed(false), Value(v) {}
        bool bOverflowed;
        T Value;
    };

    // Returns true if the product of a and b would overflow with storage type T
    template <class T, class U>
    constexpr ChkResult<U> ChkMult_(U a, U b)
    {
        if ((b > 0 && a > std::numeric_limits<T>::max() / Abs_(b)) ||
            (b < 0 && a < std::numeric_limits<T>::min() / Abs_(b)))
        {
            return {};
        }
        return a * b;
    }

    // Returns true if the sum of a and b would overflow with storage type T
    template <class T, class U>
    constexpr ChkResult<U> ChkAdd_(U a, U b)
    {
        if ((b > 0 && a > std::numeric_limits<T>::max() - b) ||
            (b < 0 && a < std::numeric_limits<T>::min() - b))
        {
            return {};
        }
        return a + b;
    }

    enum class Q32 : int32{ };
    enum class Q64 : int64{ };

    template <class> struct TFixedQ {};
    template <> struct TFixedQ<int32> { using type = Q32; };
    template <> struct TFixedQ<int64> { using type = Q64; };

    template <class T> using TFixedQ_T = typename TFixedQ<T>::type;

    template <class, class> struct TCommonQ {};

    template <> struct TCommonQ<int32, int32> { using type = int32; };
    template <> struct TCommonQ<int32, int64> { using type = int64; };
    template <> struct TCommonQ<int64, int32> { using type = int64; };
    template <> struct TCommonQ<int64, int64> { using type = int64; };

    template <class T, class U> using TCommonQ_T = typename TCommonQ<T, U>::type;

    // Calculates the number of bits needed to store the value n.
    // https://en.cppreference.com/w/cpp/numeric/bit_width
    template <class T>
    constexpr T bit_width(T n)
    {
        T un = n;
        T c = 0;
        for (; un != 0; un >>= 1) ++c;
        return static_cast<T>(c) * (n < 0 ? -1 : 1);
    }

    // TODO (jfarris): implement fixed point someday
    template <int64 Tb, class T = int32>
    struct TFixed
    {
        static_assert(Tb < (sizeof(T) << 3) - 2);
        using ValueT = T;
        static constexpr int64 D = 1LL << Tb;
        static constexpr int64 B = Tb;

        static constexpr TFixedQ_T<T> QMIN = TFixedQ_T<T>(std::numeric_limits<T>::min());
        static constexpr TFixedQ_T<T> QMAX = TFixedQ_T<T>(std::numeric_limits<T>::max());
        static constexpr TFixedQ_T<T> QSTEP = TFixedQ_T<T>(1);
        static constexpr TFixedQ_T<T> QZero = TFixedQ_T<T>(0);
        static constexpr TFixedQ_T<T> QOne = TFixedQ_T<T>(D);

        static const TFixed Epsilon;
        static const TFixed Min;
        static const TFixed Max;
        static const TFixed Step;

        template <class U> static constexpr T ToFixedValue(U v) { return static_cast<T>((int64(v * D))); }
        template <class U> static constexpr U FromFixedValue(T v) { return static_cast<U>(v) / D; }

        constexpr TFixed() : Value(0){}
        constexpr TFixed(Q32 v) : Value(static_cast<T>(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(Q64 v) : Value(static_cast<T>(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(int32 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(uint32 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(int64 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(uint64 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(float v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(double v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}

        template <int32 Ub, class U>
        constexpr TFixed(const TFixed<Ub, U>& other) : Value(ConvertToRaw(other)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}

        constexpr explicit operator Q32() const { return (Q32)FromFixedValue<double>(Value); }
        constexpr explicit operator Q64() const { return (Q64)FromFixedValue<double>(Value); }
        constexpr explicit operator int32() const { return (int32)FromFixedValue<double>(Value); }
        constexpr explicit operator uint32() const { return (uint32)FromFixedValue<double>(Value); }
        constexpr explicit operator int64() const { return (int64)FromFixedValue<double>(Value); }
        constexpr explicit operator uint64() const { return (uint64)FromFixedValue<double>(Value); }
        constexpr explicit operator float() const { return FromFixedValue<float>(Value); }
        constexpr explicit operator double() const { return FromFixedValue<double>(Value); }

        template <int32 Ub, class U>
        static constexpr T ConvertToRaw(const TFixed<Ub, U>& other)
        {
            constexpr int64 Td = 1 << Tb;
            constexpr int64 Ud = 1 << Ub;
            return T(int64(other.Value * Td / Ud));
        }

        T Value;
#if TFIXED_DEBUG
        double _Debug = 0;
#endif
    };

    template <int64 Tb, class T> const TFixed<Tb, T> TFixed<Tb, T>::Epsilon = TFixed(1E-3);
    template <int64 Tb, class T> const TFixed<Tb, T> TFixed<Tb, T>::Min = QMIN;
    template <int64 Tb, class T> const TFixed<Tb, T> TFixed<Tb, T>::Max = QMAX;
    template <int64 Tb, class T> const TFixed<Tb, T> TFixed<Tb, T>::Step = QSTEP;

#define DEF_SIGNED_FIXED(S, N) using Fixed##S##_##N = TFixed<N, int##S>

    DEF_SIGNED_FIXED(32, 0);
    DEF_SIGNED_FIXED(32, 1);
    DEF_SIGNED_FIXED(32, 2);
    DEF_SIGNED_FIXED(32, 4);
    DEF_SIGNED_FIXED(32, 8);
    DEF_SIGNED_FIXED(32, 12);
    DEF_SIGNED_FIXED(32, 16);
    DEF_SIGNED_FIXED(32, 20);
    DEF_SIGNED_FIXED(32, 24);
    DEF_SIGNED_FIXED(32, 28);
    DEF_SIGNED_FIXED(32, 30);

    DEF_SIGNED_FIXED(64, 0);
    DEF_SIGNED_FIXED(64, 1);
    DEF_SIGNED_FIXED(64, 2);
    DEF_SIGNED_FIXED(64, 4);
    DEF_SIGNED_FIXED(64, 8);
    DEF_SIGNED_FIXED(64, 12);
    DEF_SIGNED_FIXED(64, 16);
    DEF_SIGNED_FIXED(64, 20);
    DEF_SIGNED_FIXED(64, 24);
    DEF_SIGNED_FIXED(64, 28);
    DEF_SIGNED_FIXED(64, 30);
    DEF_SIGNED_FIXED(64, 32);

#undef DEF_SIGNED_FIXED

    // Useful for storing reciprocal values ie 1/X without losing precision 
    template <int32 Tb, class T = int32>
    struct TInvFixed
    {
        using ValueT = T;
        static constexpr int64 D = 1 << Tb;
        static constexpr int64 B = Tb;
        
        template <class U> static constexpr T ToFixedValue(U v) { return static_cast<T>(static_cast<double>(D) / static_cast<double>(v)); }
        template <class U> static constexpr U FromFixedValue(T v) { return U(D) / U(v); }

        constexpr TInvFixed() : Value(0) {}
        constexpr TInvFixed(Q32 v) : Value(static_cast<T>(v)) TFIXED_DEBUG_FIELD(D / static_cast<double>(Value)) {}
        constexpr TInvFixed(Q64 v) : Value(static_cast<T>(v)) TFIXED_DEBUG_FIELD(D / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(int32 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(D / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(uint32 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(D / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(int64 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(D / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(uint64 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(D / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(float v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(D / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(double v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(D / static_cast<double>(Value)) {}

        constexpr explicit operator float() const { return FromFixedValue<float>(Value); }
        constexpr explicit operator double() const { return FromFixedValue<double>(Value); }
        constexpr explicit operator TFixed<Tb, T>() const { return TFixedQ_T<T>(double(D) / Value); }

        T Value;
#if TFIXED_DEBUG
        double _Debug = 0;
#endif
    };

    template <class T>
    using TInvFixed2 = TInvFixed<T::B, typename T::ValueT>;

    template <class A, class B> constexpr auto _min(A a, B b) { return a < b ? a : b; }
    template <class A, class B> constexpr auto _max(A a, B b) { return a > b ? a : b; }

    template <int32 Tb, class T>
    struct TFixedSq
    {
        TFixed<Tb, T> Value;
    };

    template <class T>
    using TFixedSq2 = TFixedSq<T::B, T>;

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
    template<int32 Tb, class T>
    constexpr bool operator==(const TFixed<Tb, T>& lhs, const TFixed<Tb, T>& rhs)
    {
        auto v = int64(lhs.Value) - rhs.Value; 
        return (v < 0 ? -v : v) <= 1;
    }

    // TFixed<Tb, T> == TFixed<Ub, U>
    template<int32 Tb, class T, int32 Ub, class U>
    constexpr bool operator==(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        auto a = int64(lhs.Value) * (1 << Ub);
        auto b = int64(rhs.Value) * (1 << Tb);
        auto v = a - b; 
        return (v < 0 ? -v : v) <= 1;
    }

    // TFixed == double
    template<int32 Tb, class T>
    constexpr bool operator==(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs.Value == TFixed<Tb, T>::ToFixedValue(rhs);
    }

    // double == TFixed 
    template<int32 Tb, class T>
    constexpr bool operator==(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) == rhs;
    }

    // TInvFixed == TInvFixed 
    template<int32 Tb, class T>
    constexpr bool operator==(const TInvFixed<Tb, T>& lhs, const TInvFixed<Tb, T>& rhs)
    {
        return lhs.Value == rhs.Value;
    }

    // TInvFixed == double 
    template<int32 Tb, class T>
    constexpr bool operator==(const TInvFixed<Tb, T>& lhs, double rhs)
    {
        return lhs.Value == TInvFixed<Tb, T>::ToFixedValue(rhs);
    }

    // double == TInvFixed
    template<int32 Tb, class T>
    constexpr bool operator==(double lhs, const TInvFixed<Tb, T>& rhs)
    {
        return TInvFixed<Tb, T>::ToFixedValue(lhs) == rhs;
    }

    // operator!=

    template<int32 Tb, class T, int32 Ub, class U>
    constexpr bool operator!=(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        return !operator==(lhs, rhs);
    }

    template<int32 Tb, class T>
    constexpr bool operator!=(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs.Value != TFixed<Tb, T>::ToFixedValue(rhs);
    }

    template<int32 Tb, class T>
    constexpr bool operator!=(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) != rhs;
    }

    // operator<

    template<int32 Tb, class T, int32 Ub, class U>
    constexpr bool operator<(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
            return int64(lhs.Value) * Ud / Td < rhs.Value;
        else
            return lhs.Value < int64(rhs.Value) * Td / Ud;
    }

    template<int32 Tb, class T>
    constexpr bool operator<(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs < TFixed<Tb, T>(rhs);
    }

    template<int32 Tb, class T>
    constexpr bool operator<(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) < rhs;
    }

    // operator<=

    template<int32 Tb, class T, int32 Ub, class U>
    constexpr bool operator<=(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
            return int64(lhs.Value) * Ud / Td <= rhs.Value;
        else
            return lhs.Value <= int64(rhs.Value) * Td / Ud;
    }

    template<int32 Tb, class T>
    constexpr bool operator<=(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs <= TFixed<Tb, T>(rhs);
    }

    template<int32 Tb, class T>
    constexpr bool operator<=(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) <= rhs;
    }

    // operator>

    template<int32 Tb, class T, int32 Ub, class U>
    constexpr bool operator>(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
            return int64(lhs.Value) * Ud / Td > rhs.Value;
        else
            return lhs.Value > int64(rhs.Value) * Td / Ud;
    }

    template<int32 Tb, class T>
    constexpr bool operator>(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs > TFixed<Tb, T>(rhs);
    }

    template<int32 Tb, class T>
    constexpr bool operator>(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) > rhs;
    }

    // operator>=

    template<int32 Tb, class T, int32 Ub, class U>
    constexpr bool operator>=(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
            return int64(lhs.Value) * Ud / Td >= rhs.Value;
        else
            return lhs.Value >= int64(rhs.Value) * Td / Ud;
    }

    template<int32 Tb, class T>
    constexpr bool operator>=(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs >= TFixed<Tb, T>(rhs);
    }

    template<int32 Tb, class T>
    constexpr bool operator>=(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) >= rhs;
    }

    //
    // Addition
    //

    // template <int64 Tb, class T, int64 Ub, class U>
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
    template <int32 Tb, class T, int32 Ub, class U>
    constexpr auto operator+(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
        {
            return TFixed<Ub, U>(Q64(int64(lhs.Value) * Ud / Td + rhs.Value));
        }
        else
        {
            return TFixed<Tb, T>(Q64(lhs.Value + int64(rhs.Value) * Td / Ud));
        }
    }

    // TFixed + double
    template <int32 Tb, class T>
    constexpr auto operator+(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs + TFixed<Tb, T>(rhs);
    }

    // double + TFixed
    template <int32 Tb, class T>
    constexpr auto operator+(double lhs, const TFixed<Tb, T>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs + lhs;
    }

    // TFixed<Tb, T> += TFixed<Ub, U>
    template<int32 Tb, class T, int32 Ub, class U>
    constexpr auto& operator+=(TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        lhs = lhs + rhs;
        return lhs;
    }

    // TFixed += double
    template<int32 Tb, class T>
    constexpr auto& operator+=(TFixed<Tb, T>& lhs, double rhs)
    {
        lhs += TFixed<Tb, T>(rhs);
        return lhs;
    }

    // TInvFixed + TInvFixed
    template <int32 Tb, class T, int32 Ub, class U>
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
    template <int32 Tb, class T>
    constexpr auto operator+(const TInvFixed<Tb, T>& lhs, double rhs)
    {
        return TInvFixed<Tb, T>(TFixedQ_T<T>(lhs.Value + TFixed<Tb, T>::ToFixedValue(rhs)));
    }

    // double + TInvFixed
    template <int32 Tb, class T>
    constexpr auto operator+(double lhs, const TInvFixed<Tb, T>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs + lhs;
    }

    // TInvFixed += TInvFixed
    template <int32 Tb, class T>
    constexpr auto& operator+=(TInvFixed<Tb, T>& lhs, const TInvFixed<Tb, T>& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    // TInvFixed += double
    template <int32 Tb, class T>
    constexpr auto& operator+=(TInvFixed<Tb, T>& lhs, double rhs)
    {
        lhs += TFixed<Tb, T>(rhs);
        return lhs;
    }

    //
    // Subtraction
    //

    // TFixed<Tb, T> - TFixed<Ub, U>
    template <int32 Tb, class T, int32 Ub, class U>
    constexpr auto operator-(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        return lhs + -rhs;
    }

    // TFixed - double
    template <int32 Tb, class T>
    constexpr auto operator-(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs - TFixed<Tb, T>(rhs);
    }

    // double - TFixed
    template <int32 Tb, class T>
    constexpr auto operator-(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) - rhs;
    }

    // -TFixed
    template <int32 Tb, class T>
    constexpr auto operator-(const TFixed<Tb, T>& lhs)
    {
        return TFixed<Tb, T>(TFixedQ_T<T>(-lhs.Value));
    }

    // TFixed<Tb, T> -= TFixed<Ub, U>
    template<int32 Tb, class T, int32 Ub, class U>
    constexpr auto& operator-=(TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        lhs = lhs - rhs;
        return lhs;
    }

    // TFixed -= double
    template<int32 Tb, class T>
    constexpr auto& operator-=(TFixed<Tb, T>& lhs, double rhs)
    {
        lhs -= TFixed<Tb, T>(rhs);
        return lhs;
    }

    // TInvFixed - TInvFixed
    template <int32 Tb, class T, int32 Ub, class U>
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
    template<int32 Tb, class T>
    constexpr auto operator-(const TInvFixed<Tb, T>& lhs, double rhs)
    {
        return lhs - TFixed<Tb, T>(rhs);
    }

    // double - TInvFixed
    template<int32 Tb, class T>
    constexpr auto operator-(double lhs, TInvFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) - rhs;
    }

    // TInvFixed -= TInvFixed
    template <int32 Tb, class T>
    constexpr auto& operator-=(TInvFixed<Tb, T>& lhs, const TInvFixed<Tb, T>& rhs)
    {
        lhs = lhs - rhs;
        return lhs;
    }

    // TInvFixed -= double
    template<int32 Tb, class T>
    constexpr auto& operator-=(TInvFixed<Tb, T>& lhs, double rhs)
    {
        lhs -= TInvFixed<Tb, T>(TFixedQ_T<T>(TFixed<Tb, T>::ToFixedValue(rhs)));
        return lhs;
    }

    //
    // Multiplication
    //

    // template <int64 Tb, class T, int64 Ub, class U>
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
    template<int32 Tb, class T, int32 Ub, class U>
    constexpr auto operator*(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr auto MIN = Tb < Ub ? Tb : Ub;
        constexpr auto MAX = Tb > Ub ? Tb : Ub;
        return TFixed<MAX, T>(Q64((int64(lhs.Value) * rhs.Value) >> MIN));
    }

    // TFixed * double
    template<int32 Tb, class T>
    constexpr auto operator*(const TFixed<Tb, T>& lhs, double rhs)
    {
        return lhs * TFixed<Tb, T>(rhs);
    }

    // double * TFixed
    template <int32 Tb, class T>
    constexpr auto operator*(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) * rhs;
    }

    // TFixed<Tb, T> *= TFixed<Ub, U>
    template<int32 Tb, class T, int32 Ub, class U>
    constexpr auto& operator*=(TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }

    // TFixed *= double
    template<int32 Tb, class T>
    constexpr auto& operator*=(TFixed<Tb, T>& lhs, double rhs)
    {
        lhs *= TFixed<Tb, T>(rhs);
        return lhs;
    }

    // TFixed<Tb, T> * TInvFixed<Ub, U>
    // X*(1/Y) is just X/Y
    template <int32 Tb, class T, int32 Ub, class U>
    constexpr auto operator*(const TFixed<Tb, T>& lhs, const TInvFixed<Ub, U>& rhs)
    {
        return lhs / TFixed<Ub, U>(TFixedQ_T<U>(rhs.Value));
    }

    // TInvFixed<Tb, T> * TFixed<Ub, U>
    // (1/X)*Y is just Y/X
    template <int32 Tb, class T, int32 Ub, class U>
    constexpr auto operator*(const TInvFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs * lhs;
    }

    // TInvFixed * TInvFixed
    // (1/X)*(1/Y) is just 1/(X*Y)
    template <int32 Tb, class T>
    constexpr auto operator*(const TInvFixed<Tb, T>& lhs, const TInvFixed<Tb, T>& rhs)
    {
        return TInvFixed<Tb, T>(Q64(int64(lhs.Value) * (1 << Tb) * rhs.Value));
    }

    //
    // Division
    //

    // TFixed<Tb, T> / TFixed<Ub, U>
    template<int32 Tb, class T, int32 Ub, class U>
    constexpr auto operator/(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
        {
            return TFixed<Ub, T>(Q64(lhs.Value / int64(rhs.Value) * Td));
        }
        else
        {
            return TFixed<Tb, T>(Q64(int64(lhs.Value) * Ud / rhs.Value));
        }
    }

    // TFixed / double
    template <int32 Tb, class T>
    constexpr auto operator/(const TFixed<Tb, T>& lhs, double rhs)
    {
        return TFixed<Tb, T>(Q64(lhs.Value / rhs));
    }

    // double / TFixed
    template <int32 Tb, class T>
    constexpr auto operator/(double lhs, const TFixed<Tb, T>& rhs)
    {
        return TFixed<Tb, T>(lhs) / rhs;
    }

    // TFixed<Tb, T> /= TFixed<Ub, U>
    template<int32 Tb, class T, int32 Ub, class U>
    constexpr auto& operator/=(TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        lhs = lhs / rhs;
        return lhs;
    }

    // TFixed /= double
    template<int32 Tb, class T>
    constexpr auto& operator/=(TFixed<Tb, T>& lhs, double rhs)
    {
        lhs = lhs / TFixed<Tb, T>(rhs);
        return lhs;
    }

    // TFixed<Tb, T> / TInvFixed<Ub, U>
    // X/(1/Y) is just X*Y
    template <int32 Tb, class T, int32 Ub, class U>
    constexpr auto operator/(const TFixed<Tb, T>& lhs, const TInvFixed<Ub, U>& rhs)
    {
        return lhs * TFixed<Ub, U>(TFixedQ_T<U>(rhs.Value));
    }

    // TInvFixed<Tb, T> / TFixed<Ub, U>
    // (1/X)/Y is just 1/(X*Y)
    template <int32 Tb, class T, int32 Ub, class U>
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

    template <int32 Tb, class T>
    constexpr static TFixed<Tb, T> Abs(const TFixed<Tb, T>& value)
    {
        return TFixedQ_T<T>(Abs_(value.Value));
    }

    template <class T>
    constexpr TInvFixed<T::B, typename T::ValueT> OneDivBy(const T& v)
    {
        return TFixedQ_T<typename T::ValueT>(v.Value);
    }

    template <int32 Tb, class T>
    constexpr TFixed<Tb, T> OneDivBy(const TInvFixed<Tb, T>& v)
    {
        return TFixedQ_T<T>(v.Value);
    }
}
