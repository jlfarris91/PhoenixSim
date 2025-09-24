#pragma once

#include <cstdint>

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
    typedef int8_t int8;
    typedef int16_t int16;
    typedef int32_t int32;
    typedef int64_t int64;
    typedef uint8_t uint8;
    typedef uint16_t uint16;
    typedef uint32_t uint32;
    typedef uint64_t uint64;

    enum class Q32 : int32{ };
    enum class Q64 : int64{ };

    template <class> struct TFixedQ {};
    template <> struct TFixedQ<int32> { using type = Q32; };
    template <> struct TFixedQ<int64> { using type = Q64; };

    template <class T> using TFixedQ_T = typename TFixedQ<T>::type;

    template <class, class> struct TLargestQ {};

    template <> struct TLargestQ<int32, int32> { using type = int32; };
    template <> struct TLargestQ<int32, int64> { using type = int64; };
    template <> struct TLargestQ<int64, int32> { using type = int64; };
    template <> struct TLargestQ<int64, int64> { using type = int64; };

    template <class T, class U> using TLargestQ_T = typename TLargestQ<T, U>::type;

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
        using ValueT = T;
        static constexpr int64 D = 1 << Tb;
        static constexpr int64 B = Tb;

        static const TFixed EPSILON;
        static const TFixed MIN;
        static const TFixed MAX;

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
            if constexpr (Tb < Ub)
                return T(int64(other.Value) * Td / Ud);
            else
                return T(int64(other.Value) * Ud / Td);
        }

        T Value;
#if TFIXED_DEBUG
        double _Debug = 0;
#endif
    };

    template <int64 Tb, class T> const TFixed<Tb, T> TFixed<Tb, T>::EPSILON = TFixed(1E-3);

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
        constexpr explicit TInvFixed(const TFixed<Tb, T>& v) : Value(v.Value) TFIXED_DEBUG_FIELD(D / static_cast<double>(Value)) {}

        constexpr explicit operator float() const { return FromFixedValue<float>(Value); }
        constexpr explicit operator double() const { return FromFixedValue<double>(Value); }

        T Value;
#if TFIXED_DEBUG
        double _Debug = 0;
#endif
    };

    template <class T>
    using TInvFixed2 = TInvFixed<T::B, typename T::ValueT>;

    template <class T>
    constexpr TInvFixed<T::B, typename T::ValueT> OneDivBy(const T& v)
    {
        return TFixedQ_T<typename T::ValueT>(v.Value);
    }

    template <int32 Tb, class T>
    constexpr TInvFixed<Tb, T> OneDivBy(const TInvFixed<Tb, T>& v)
    {
        return OneDivBy(TFixed<Tb, T>(TFixedQ_T<T>(v.Value)));
    }

    template <class A, class B>
    constexpr auto _min(A a, B b) { return a < b ? a : b; }

    // Helper for defining the precision for a squared number
    template <uint64 Tb, class T> using TFixedSq_ = TFixed<_min(Tb+Tb, (sizeof(T) << 3 - 2)), int64>;
    template <class T> using TFixedSq = TFixedSq_<T::B, int64>;

    using Fixed4 = TFixed<4>;
    using Fixed8 = TFixed<8>;
    using Fixed12 = TFixed<12>;
    using Fixed16 = TFixed<16>;
    using Fixed20 = TFixed<20>;
    using Fixed24 = TFixed<24>;
    using Fixed28 = TFixed<28>;

#ifndef FP_NO_STANDARD_TYPES
    using Value = TFixed<12>;
    using ValueSq = TFixedSq<Value>;
    using Distance = TFixed<16>;
    using DistanceSq = TFixedSq<Distance>;
    using Time = Fixed4;
    using DeltaTime = TInvFixed2<Time>;
    using Speed = Fixed16;
    using Angle = Fixed20;
#endif

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
        return lhs.Value == rhs.Value;
    }

    // TFixed<Tb, T> == TFixed<Ub, U>
    template<int32 Tb, class T, int32 Ub, class U>
    constexpr bool operator==(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        auto a = int64(lhs.Value) * (1 << Ub);
        auto b = int64(rhs.Value) * (1 << Tb);
        return a == b;
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
        lhs.Value += TFixed<Tb, T>::ToFixedValue(rhs);
        return lhs;
    }

    // TInvFixed + TInvFixed
    template <int32 Tb, class T, int32 Ub, class U>
    constexpr auto operator+(const TInvFixed<Tb, T>& lhs, const TInvFixed<Ub, U>& rhs)
    {
        constexpr auto Vb = Tb > Ub ? Tb : Ub;
        constexpr auto Vd = Tb > Ub ? Tb : Ub;
        using V = TLargestQ_T<T, U>;
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
        lhs.Value += rhs.Value;
        return lhs;
    }

    // TInvFixed += double
    template <int32 Tb, class T>
    constexpr auto& operator+=(TInvFixed<Tb, T>& lhs, double rhs)
    {
        lhs.Value += TFixed<Tb, T>::ToFixedValue(rhs);
        return lhs;
    }

    //
    // Subtraction
    //

    // TFixed<Tb, T> - TFixed<Ub, U>
    template <int32 Tb, class T, int32 Ub, class U>
    constexpr auto operator-(const TFixed<Tb, T>& lhs, const TFixed<Ub, U>& rhs)
    {
        constexpr int64 Td = 1 << Tb;
        constexpr int64 Ud = 1 << Ub;
        if constexpr (Tb < Ub)
        {
            return TFixed<Ub, U>(Q64(int64(lhs.Value) * Ud / Td - rhs.Value));
        }
        else
        {
            return TFixed<Tb, T>(Q64(lhs.Value - int64(rhs.Value) * Td / Ud));
        }
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
        lhs.Value -= TFixed<Tb, T>::ToFixedValue(rhs);
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
        return TFixed<Tb, T>(Q64(int64(lhs.Value) * rhs));
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
        lhs.Value *= TFixed<Tb, T>::ToFixedValue(rhs);
        return lhs;
    }

    // TFixed<Tb, T> * TInvFixed<Ub, U>
    // X*(1/Y) is just X/Y
    template <int32 Tb, class T, int32 Ub, class U>
    constexpr auto operator*(const TFixed<Tb, T>& lhs, const TInvFixed<Ub, U>& rhs)
    {
        return TFixed<Tb, T>(Q64(int64(lhs.Value) * (1 << Ub) / rhs.Value));
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
        lhs.Value = lhs.Value / rhs;
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
        return rhs.Value * lhs.Value;
    }
}
