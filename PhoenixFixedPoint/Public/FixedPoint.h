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
    constexpr int32 bit_width(int32 n)
    {
        uint32 un = n;
        uint32 c = 0;
        for (; un != 0; un >>= 1) ++c;
        return (int32)c * (n < 0 ? -1 : 1);
    }

    // TODO (jfarris): implement fixed point someday
    template <int64 Td, class T = int32>
    struct TFixed
    {
        using StorageT = T;
        static constexpr int32 D = Td;
        static constexpr int32 B = bit_width(Td - 1);

        template <class U> static constexpr T ToFixedValue(U v) { return static_cast<T>(v * Td); }
        template <class U> static constexpr U FromFixedValue(T v) { return static_cast<U>(v) / Td; }

        constexpr TFixed() : Value(0){}
        constexpr TFixed(Q32 v) : Value(static_cast<T>(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / Td) {}
        constexpr TFixed(Q64 v) : Value(static_cast<T>(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / Td) {}
        constexpr TFixed(int32 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / Td) {}
        constexpr TFixed(uint32 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / Td) {}
        constexpr TFixed(int64 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / Td) {}
        constexpr TFixed(uint64 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / Td) {}
        constexpr TFixed(float v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / Td) {}
        constexpr TFixed(double v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / Td) {}

        template <int64 Ud, class U>
        constexpr TFixed(const TFixed<Ud, U>& other) : Value(ConvertToRaw(other)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / Td) {}

        constexpr explicit operator int32() const { return (int32)FromFixedValue<double>(Value); }
        constexpr explicit operator uint32() const { return (uint32)FromFixedValue<double>(Value); }
        constexpr explicit operator int64() const { return (int64)FromFixedValue<double>(Value); }
        constexpr explicit operator uint64() const { return (uint64)FromFixedValue<double>(Value); }
        constexpr explicit operator float() const { return FromFixedValue<float>(Value); }
        constexpr explicit operator double() const { return FromFixedValue<double>(Value); }

        template <int64 Ud, class U>
        static constexpr T ConvertToRaw(const TFixed<Ud, U>& other)
        {
            if constexpr (Td < Ud)
                return T(int64(other.Value) / (Ud / Td));
            else
                return T(int64(other.Value) * (Td / Ud));
        }

        T Value;
#if TFIXED_DEBUG
        double _Debug = 0;
#endif
    };

    using TFixedValue = TFixed<1024>;

    // Useful for storing reciprocal values ie 1/X 
    template <int64 Td, class T = int32>
    struct TInvFixed
    {
        using StorageT = T;
        static constexpr int32 D = Td;
        static constexpr int32 B = bit_width(Td - 1);
        
        template <class U> static constexpr T ToFixedValue(U v) { return static_cast<T>(static_cast<double>(Td) / static_cast<double>(v)); }
        template <class U> static constexpr U FromFixedValue(T v) { return U(Td) / U(v); }

        constexpr TInvFixed() : Value(0) {}
        constexpr TInvFixed(Q32 v) : Value(static_cast<T>(v)) TFIXED_DEBUG_FIELD(Td / static_cast<double>(Value)) {}
        constexpr TInvFixed(Q64 v) : Value(static_cast<T>(v)) TFIXED_DEBUG_FIELD(Td / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(int32 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(Td / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(uint32 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(Td / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(int64 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(Td / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(uint64 v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(Td / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(float v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(Td / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(double v) : Value(ToFixedValue(v)) TFIXED_DEBUG_FIELD(Td / static_cast<double>(Value)) {}
        constexpr explicit TInvFixed(const TFixed<Td, T>& v) : Value(v.Value) TFIXED_DEBUG_FIELD(Td / static_cast<double>(Value)) {}

        constexpr explicit operator float() const { return FromFixedValue<float>(Value); }
        constexpr explicit operator double() const { return FromFixedValue<double>(Value); }

        T Value;
#if TFIXED_DEBUG
        double _Debug = 0;
#endif
    };

    template <class T>
    using TInvFixed2 = TInvFixed<T::D, typename T::StorageT>;

    using TInvFixedValue = TInvFixed2<TFixedValue>;

    template <class T>
    constexpr TInvFixed<T::D, typename T::StorageT> OneDivBy(const T& v)
    {
        return TFixedQ_T<typename T::StorageT>(v.Value);
    }

    template <int64 Td, class T>
    constexpr TInvFixed<Td, T> OneDivBy(const TInvFixed<Td, T>& v)
    {
        return OneDivBy(TFixed<Td, T>(TFixedQ_T<T>(v.Value)));
    }

    template <class> struct TLiteral;
    template <> struct TLiteral<double>
    {
        TLiteral(double v) : Value(v) {}
        double Value = 0.0f;
    };

    template <> struct TLiteral<float>
    {
        TLiteral(float v) : Value(v) {}
        float Value = 0.0f;
    }; 

    static_assert(bit_width(1) == 1);
    static_assert(bit_width(3) == 2);
    static_assert(bit_width(5) == 3);
    static_assert(bit_width(64) == 7);

    using Value = TFixedValue;
    using Distance = TFixed<16 * 1024>;
    using Distance2 = TFixed<Distance::D, int64>;
    using Time = TFixed<64>;
    using DeltaTime = TInvFixed2<Time>;
    using Speed = TFixed<1024>;
    using Angle = TFixed<0x10000000 / 45>;

    static_assert(Value::D == 1024);
    static_assert(Value::B == 10);

    static_assert(Value(0.5f).Value == 512);
    static_assert(Value(1.0f).Value == 1024);
    static_assert(Value(2.0f).Value == 2048);

    static_assert(Value(1.0f).Value == Value::ToFixedValue(1.0f));
    static_assert(1.0f == Value::FromFixedValue<float>(1024));

    static_assert((int32)Value(123.123f) == 123);
    static_assert((uint32)Value(123.123f) == 123);
    static_assert((int64)Value(123.123f) == 123);
    static_assert((uint64)Value(123.123f) == 123);

    static_assert(Angle(1).Value == 0x10000000 / 45);
    static_assert(Angle(45).Value == (0x10000000 / 45) * 45);

    ///////////////////////////////////////////////////////////////////////////
    // 
    // Operations
    //
    ///////////////////////////////////////////////////////////////////////////

    //
    // Equality
    //

    // operator==

    // TFixed<Td, T> == TFixed<Td, T>
    template<int64 Td, class T>
    constexpr bool operator==(const TFixed<Td, T>& lhs, const TFixed<Td, T>& rhs)
    {
        return lhs.Value == rhs.Value;
    }

    // TFixed<Td, T> == TFixed<Ud, U>
    template<int64 Td, class T, int64 Ud, class U>
    constexpr bool operator==(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        auto a = int64(lhs.Value) * Ud;
        auto b = int64(rhs.Value) * Td;
        return a == b;
    }

    // TFixed == double
    template<int64 Td, class T>
    constexpr bool operator==(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs.Value == TFixed<Td, T>::ToFixedValue(rhs);
    }

    // double == TFixed 
    template<int64 Td, class T>
    constexpr bool operator==(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) == rhs;
    }

    // TInvFixed == TInvFixed 
    template<int64 Td, class T>
    constexpr bool operator==(const TInvFixed<Td, T>& lhs, const TInvFixed<Td, T>& rhs)
    {
        return lhs.Value == rhs.Value;
    }

    // TInvFixed == double 
    template<int64 Td, class T>
    constexpr bool operator==(const TInvFixed<Td, T>& lhs, double rhs)
    {
        return lhs.Value == TInvFixed<Td, T>::ToFixedValue(rhs);
    }

    // double == TInvFixed
    template<int64 Td, class T>
    constexpr bool operator==(double lhs, const TInvFixed<Td, T>& rhs)
    {
        return TInvFixed<Td, T>::ToFixedValue(lhs) == rhs;
    }

    // operator!=

    template<int64 Td, class T, int64 Ud, class U>
    constexpr bool operator!=(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        return !operator==(lhs, rhs);
    }

    template<int64 Td, class T>
    constexpr bool operator!=(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs.Value != TFixed<Td, T>::ToFixedValue(rhs);
    }

    template<int64 Td, class T>
    constexpr bool operator!=(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) != rhs;
    }

    // operator<

    template<int64 Td, class T, int64 Ud, class U>
    constexpr bool operator<(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        if constexpr (Td > Ud)
            return lhs.Value < rhs.Value * (Td / Ud);
        else
            return lhs.Value * (Ud / Td) < rhs.Value;
    }

    template<int64 Td, class T>
    constexpr bool operator<(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs < TFixed<Td, T>(rhs);
    }

    template<int64 Td, class T>
    constexpr bool operator<(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) < rhs;
    }

    // operator<=

    template<int64 Td, class T, int64 Ud, class U>
    constexpr bool operator<=(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        if constexpr (Td > Ud)
            return lhs.Value <= rhs.Value * (Td / Ud);
        else
            return lhs.Value * (Ud / Td) <= rhs.Value;
    }

    template<int64 Td, class T>
    constexpr bool operator<=(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs <= TFixed<Td, T>(rhs);
    }

    template<int64 Td, class T>
    constexpr bool operator<=(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) <= rhs;
    }

    // operator>

    template<int64 Td, class T, int64 Ud, class U>
    constexpr bool operator>(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        if constexpr (Td > Ud)
            return lhs.Value > rhs.Value * (Td / Ud);
        else
            return lhs.Value * (Ud / Td) > rhs.Value;
    }

    template<int64 Td, class T>
    constexpr bool operator>(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs > TFixed<Td, T>(rhs);
    }

    template<int64 Td, class T>
    constexpr bool operator>(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) > rhs;
    }

    // operator>=

    template<int64 Td, class T, int64 Ud, class U>
    constexpr bool operator>=(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        if constexpr (Td > Ud)
            return lhs.Value >= rhs.Value * (Td / Ud);
        else
            return lhs.Value * (Ud / Td) >= rhs.Value;
    }

    template<int64 Td, class T>
    constexpr bool operator>=(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs >= TFixed<Td, T>(rhs);
    }

    template<int64 Td, class T>
    constexpr bool operator>=(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) >= rhs;
    }

    //
    // Addition
    //

    // TFixed<Td, T> + TFixed<Ud, U>
    template <int64 Td, class T, int64 Ud, class U>
    constexpr auto operator+(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        if constexpr (Td > Ud)
        {
            return TFixed<Td, T>(static_cast<Q64>(lhs.Value + rhs.Value * (Td / Ud)));
        }
        else
        {
            return TFixed<Ud, U>(static_cast<Q64>(lhs.Value * (Ud / Td) + rhs.Value));
        }
    }

    // TFixed + double
    template <int64 Td, class T>
    constexpr auto operator+(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs + TFixed<Td, T>(rhs);
    }

    // double + TFixed
    template <int64 Td, class T>
    constexpr auto operator+(double lhs, const TFixed<Td, T>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs + lhs;
    }

    // TFixed<Td, T> += TFixed<Ud, U>
    template<int64 Td, class T, int64 Ud, class U>
    constexpr auto& operator+=(TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        lhs = lhs + rhs;
        return lhs;
    }

    // TFixed += double
    template<int64 Td, class T>
    constexpr auto& operator+=(TFixed<Td, T>& lhs, double rhs)
    {
        lhs.Value += TFixed<Td, T>::ToFixedValue(rhs);
        return lhs;
    }

    // TInvFixed + TInvFixed
    template <int64 Td, class T, int64 Ud, class U>
    constexpr auto operator+(const TInvFixed<Td, T>& lhs, const TInvFixed<Ud, U>& rhs)
    {
        constexpr auto Tb = TInvFixed<Td, T>::B;
        constexpr auto Ub = TInvFixed<Ud, U>::B;
        constexpr auto Vb = Tb > Ub ? Tb : Ub;
        constexpr auto Vd = Td > Ud ? Td : Ud;
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
    template <int64 Td, class T>
    constexpr auto operator+(const TInvFixed<Td, T>& lhs, double rhs)
    {
        return TInvFixed<Td, T>(TFixedQ_T<T>(lhs.Value + TFixed<Td, T>::ToFixedValue(rhs)));
    }

    // double + TInvFixed
    template <int64 Td, class T>
    constexpr auto operator+(double lhs, const TInvFixed<Td, T>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs + lhs;
    }

    // TInvFixed += TInvFixed
    template <int64 Td, class T>
    constexpr auto& operator+=(TInvFixed<Td, T>& lhs, const TInvFixed<Td, T>& rhs)
    {
        lhs.Value += rhs.Value;
        return lhs;
    }

    // TInvFixed += double
    template <int64 Td, class T>
    constexpr auto& operator+=(TInvFixed<Td, T>& lhs, double rhs)
    {
        lhs.Value += TFixed<Td, T>::ToFixedValue(rhs);
        return lhs;
    }

    static_assert(Value(1.0f) + Value(1.0f) == 2);
    static_assert(Value(1.0f) + Value(1.0f) == 2.0f);
    static_assert(Value(1.0f) + Value(1.0f) == Value(2.0f));

    static_assert(Value(1.0f) + 1.0f == 2);
    static_assert(Value(1.0f) + 1.0f == 2.0f);
    static_assert(Value(1.0f) + 1.0f == Value(2.0f));

    static_assert(1.0f + Value(1.0f) == 2);
    static_assert(1.0f + Value(1.0f) == 2.0f);
    static_assert(1.0f + Value(1.0f) == Value(2.0f));

    //
    // Subtraction
    //

    // TFixed<Td, T> - TFixed<Ud, U>
    template <int64 Td, class T, int64 Ud, class U>
    constexpr auto operator-(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        if constexpr (Td > Ud)
        {
            return TFixed<Td, T>(static_cast<Q64>(lhs.Value - rhs.Value * (Td / Ud)));
        }
        else
        {
            return TFixed<Ud, U>(static_cast<Q64>(lhs.Value * (Ud / Td) - rhs.Value));
        }
    }

    // TFixed - double
    template <int64 Td, class T>
    constexpr auto operator-(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs - TFixed<Td, T>(rhs);
    }

    // double - TFixed
    template <int64 Td, class T>
    constexpr auto operator-(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) - rhs;
    }

    // -TFixed
    template <int64 Td, class T>
    constexpr auto operator-(const TFixed<Td, T>& lhs)
    {
        return TFixed<Td, T>(TFixedQ_T<T>(-lhs.Value));
    }

    // TFixed<Td, T> -= TFixed<Ud, U>
    template<int64 Td, class T, int64 Ud, class U>
    constexpr auto& operator-=(TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        lhs = lhs - rhs;
        return lhs;
    }

    // TFixed -= double
    template<int64 Td, class T>
    constexpr auto& operator-=(TFixed<Td, T>& lhs, double rhs)
    {
        lhs.Value -= TFixed<Td, T>::ToFixedValue(rhs);
        return lhs;
    }

    // TInvFixed - TInvFixed
    template <int64 Td, class T, int64 Ud, class U>
    constexpr auto operator-(const TInvFixed<Td, T>& lhs, const TInvFixed<Ud, U>& rhs)
    {
        constexpr auto Tb = TInvFixed<Td, T>::B;
        constexpr auto Ub = TInvFixed<Ud, U>::B;
        constexpr auto Vb = Tb > Ub ? Tb : Ub;
        constexpr auto Vd = Td > Ud ? Td : Ud;
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
    template<int64 Td, class T>
    constexpr auto operator-(const TInvFixed<Td, T>& lhs, double rhs)
    {
        return lhs - TFixed<Td, T>(rhs);
    }

    // double - TInvFixed
    template<int64 Td, class T>
    constexpr auto operator-(double lhs, TInvFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) - rhs;
    }

    // TInvFixed -= TInvFixed
    template <int64 Td, class T>
    constexpr auto& operator-=(TInvFixed<Td, T>& lhs, const TInvFixed<Td, T>& rhs)
    {
        lhs = lhs - rhs;
        return lhs;
    }

    // TInvFixed -= double
    template<int64 Td, class T>
    constexpr auto& operator-=(TInvFixed<Td, T>& lhs, double rhs)
    {
        lhs -= TInvFixed<Td, T>(TFixedQ_T<T>(TFixed<Td, T>::ToFixedValue(rhs)));
        return lhs;
    }

    static_assert(Value(10.0f) - Value(5.0f) == 5);
    static_assert(Value(10.0f) - Value(5.0f) == 5.0f);
    static_assert(Value(10.0f) - Value(5.0f) == Value(5.0f));

    static_assert(Value(5.0f) - Value(10.0f) == -5);
    static_assert(Value(5.0f) - Value(10.0f) == -5.0f);
    static_assert(Value(5.0f) - Value(10.0f) == Value(-5.0f));

    static_assert(Value(10.0f) - 5.0f == 5);
    static_assert(Value(10.0f) - 5.0f == 5.0f);
    static_assert(Value(10.0f) - 5.0f == Value(5.0f));

    static_assert(Value(5.0f) - 10.0f == -5);
    static_assert(Value(5.0f) - 10.0f == -5.0f);
    static_assert(Value(5.0f) - 10.0f == Value(-5.0f));

    static_assert(10.0f - Value(5.0f) == 5);
    static_assert(10.0f - Value(5.0f) == 5.0f);
    static_assert(10.0f - Value(5.0f) == Value(5.0f));

    static_assert(5.0f - Value(10.0f) == -5);
    static_assert(5.0f - Value(10.0f) == -5.0f);
    static_assert(5.0f - Value(10.0f) == Value(-5.0f));

    //
    // Multiplication
    //

    // TFixed<Td, T> * TFixed<Ud, U>
    template<int64 Td, class T, int64 Ud, class U>
    constexpr auto operator*(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        auto v = int64(lhs.Value) * rhs.Value;
        v = v >> TFixed<Ud, U>::B;
        return TFixed<Td, int64>(Q64(v));
    }

    // TFixed * double
    template<int64 Td, class T>
    constexpr auto operator*(const TFixed<Td, T>& lhs, double rhs)
    {
        return TFixed<Td, T>(Q64(int64(lhs.Value) * rhs));
    }

    // double * TFixed
    template <int64 Td, class T>
    constexpr auto operator*(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) * rhs;
    }

    // TFixed<Td, T> *= TFixed<Ud, U>
    template<int64 Td, class T, int64 Ud, class U>
    constexpr auto& operator*=(TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }

    // TFixed *= double
    template<int64 Td, class T>
    constexpr auto& operator*=(TFixed<Td, T>& lhs, double rhs)
    {
        lhs.Value *= TFixed<Td, T>::ToFixedValue(rhs);
        return lhs;
    }

    // TFixed<Td, T> * TInvFixed<Ud, U>
    // X*(1/Y) is just X/Y
    template <int64 Td, class T, int64 Ud, class U>
    constexpr auto operator*(const TFixed<Td, T>& lhs, const TInvFixed<Ud, U>& rhs)
    {
        return TFixed<Td, T>(Q64(int64(lhs.Value) * Ud / rhs.Value));
    }

    // TInvFixed<Td, T> * TFixed<Ud, U>
    // (1/X)*Y is just Y/X
    template <int64 Td, class T, int64 Ud, class U>
    constexpr auto operator*(const TInvFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs * lhs;
    }

    // TInvFixed * TInvFixed
    // (1/X)*(1/Y) is just 1/(X*Y)
    template <int64 Td, class T>
    constexpr auto operator*(const TInvFixed<Td, T>& lhs, const TInvFixed<Td, T>& rhs)
    {
        return TInvFixed<Td, T>(Q64(int64(lhs.Value) * Td * rhs.Value));
    }

    static_assert(Value(Q32(512)) * Value(1024) == 512);

    static_assert(Value(5.0f) * Value(2.0f) == 10);
    static_assert(Value(5.0f) * Value(2.0f) == 10.0f);
    static_assert(Value(5.0f) * Value(2.0f) == Value(10.0f));

    static_assert(Value(1.0f) * 2 == 2);
    static_assert(Value(5.0f) * 2.0f == 10);
    static_assert(Value(5.0f) * 2.0f == 10.0f);
    static_assert(Value(5.0f) * 2.0f == Value(10.0f));

    static_assert(5.0f * Value(2.0f) == 10);
    static_assert(5.0f * Value(2.0f) == 10.0f);
    static_assert(5.0f * Value(2.0f) == Value(10.0f));

    static_assert(TInvFixed2<Value>(Value(10.0f)).Value == 10240);
    static_assert((Value(10) * TInvFixed2<Value>(0.1f)) == 1.0f);

    //
    // Division
    //

    // TFixed<Td, T> / TFixed<Ud, U>
    template<int64 Td, class T, int64 Ud, class U>
    constexpr auto operator/(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        return TFixed<Td, T>(Q64(int64(lhs.Value) * Ud / rhs.Value));
    }

    // TFixed / double
    template <int64 Td, class T>
    constexpr auto operator/(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs / TFixed<Td, T>(rhs);
    }

    // double / TFixed
    template <int64 Td, class T>
    constexpr auto operator/(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) / rhs;
    }

    // TFixed<Td, T> /= TFixed<Ud, U>
    template<int64 Td, class T, int64 Ud, class U>
    constexpr auto& operator/=(TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        lhs = lhs / rhs;
        return lhs;
    }

    // TFixed /= double
    template<int64 Td, class T>
    constexpr auto& operator/=(TFixed<Td, T>& lhs, double rhs)
    {
        lhs = lhs / rhs;
        return lhs;
    }

    // TFixed<Td, T> / TInvFixed<Ud, U>
    // X/(1/Y) is just X*Y
    template <int64 Td, class T, int64 Ud, class U>
    constexpr auto operator/(const TFixed<Td, T>& lhs, const TInvFixed<Ud, U>& rhs)
    {
        return lhs * TFixed<Ud, U>(TFixedQ_T<U>(rhs.Value));
    }

    // TInvFixed<Td, T> / TFixed<Ud, U>
    // (1/X)/Y is just 1/(X*Y)
    template <int64 Td, class T, int64 Ud, class U>
    constexpr auto operator/(const TInvFixed<Td, T>& lhs, const TInvFixed<Ud, U>& rhs)
    {
        // Commutative, just flip order of operators
        return rhs.Value * lhs.Value;
    }

    constexpr Value test10 = Value(10.0f);
    constexpr Value test1 = Value(1.0f);
    constexpr Value test2 = Value(2.0f);
    constexpr Value test5 = Value(5.0f);
    constexpr auto asdf = test10 / test2;

    static_assert((test10 / test2).Value == test5.Value);

    static_assert(Value(10.0f) / Value(2.0f) == 5.0f);
    static_assert(Value(10.0f) / Value(2.0f) == 5.0f);
    static_assert(Value(10.0f) / Value(2.0f) == Value(5.0f));

    constexpr Angle test90 = Angle(90);
    constexpr auto asdfsadf = test90 / test2;

    static_assert(Angle(90.0f) / Value(2.0f) == 45);

    constexpr Value testNeg1 = -1;
    constexpr Value testNeg2 = -2;
    constexpr auto asfdf = testNeg1 / test2;
    constexpr auto asfd2f = test1 / testNeg2;

    static_assert(Value(1.0f) / 2.0f == 0.5f);
    static_assert(Value(1.5f) / 2.0f == 0.75f);
    static_assert(Value(-1.0f) / 2.0f == -0.5f);
    static_assert(Value(-1.5f) / 2.0f == -0.75f);
    static_assert(Value(1.0f) / -2.0f == -0.5f);
    static_assert(Value(1.5f) / -2.0f == -0.75f);

    static_assert(OneDivBy<Value>(10.0f).Value == 10240);
    static_assert(Value(10) / OneDivBy<Value>(10.0f) == 100.0f);

    constexpr auto a = OneDivBy<Value>(2);
    constexpr auto b = OneDivBy<Value>(4);
    constexpr auto c = int64(b.Value) - a.Value;
    constexpr auto d = int64(a.Value) * b.Value;
    constexpr auto e = d / c;
    constexpr auto f = OneDivBy<Value>(2) + OneDivBy<Distance>(2);
    constexpr auto g = OneDivBy<Value>(2) - OneDivBy<Distance>(2);
    static_assert((OneDivBy<Value>(2) + OneDivBy<Value>(4)).Value == Value(1.3333f).Value);
    static_assert((OneDivBy<Value>(2) - OneDivBy<Value>(2)).Value == 0);
    static_assert((OneDivBy<Value>(2) - OneDivBy<Value>(4)).Value == Value(4.0f).Value);

    inline void test()
    {
        Value value = 1.0f;
        Distance distance = 1.0f;
        Speed speed = 1.0f;

        value = value + 1.0f;
        value += 1.0f;
        value = value - 1.0f;
        value -= 1.0f;
        value = value * 2.0f;
        value *= 2.0f;
        value = value / 2.0f;
        value /= 2.0f;
        
        distance += 1.0f;
    }
}
