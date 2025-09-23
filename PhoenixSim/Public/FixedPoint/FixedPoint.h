#pragma once

#include "DLLExport.h"
#include "PhoenixSim.h"

namespace Phoenix
{
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
    PHOENIXSIM_API constexpr int32 bit_width(uint32 n)
    {
        int32 c = 0;
        for (; n != 0; n >>= 1) ++c;
        return c;
    }
    
    // TODO (jfarris): implement fixed point someday
    template <uint32 Td, class T = int32>
    struct TFixed
    {
        using StorageT = T;
        static constexpr uint32 D = Td;
        static constexpr uint32 FracBits = bit_width(Td - 1);
        static constexpr int32 Scalar = (1 << FracBits);

        template <class U> static constexpr T ToFixedValue(U v) { return static_cast<T>(v * Td); }
        template <class U> static constexpr U FromFixedValue(T v) { return static_cast<U>(v) / Td; }

        constexpr TFixed() : Value(0){}
        constexpr TFixed(Q32 v) : Value(static_cast<T>(v)), _Debug(static_cast<double>(Value) / Td) {}
        constexpr TFixed(Q64 v) : Value(static_cast<T>(v)), _Debug(static_cast<double>(Value) / Td) {}
        constexpr TFixed(int32 v) : Value(ToFixedValue(v)), _Debug(static_cast<double>(Value) / Td) {}
        constexpr TFixed(uint32 v) : Value(ToFixedValue(v)), _Debug(static_cast<double>(Value) / Td) {}
        constexpr TFixed(int64 v) : Value(ToFixedValue(v)), _Debug(static_cast<double>(Value) / Td) {}
        constexpr TFixed(uint64 v) : Value(ToFixedValue(v)), _Debug(static_cast<double>(Value) / Td) {}
        constexpr TFixed(float v) : Value(ToFixedValue(v)), _Debug(static_cast<double>(Value) / Td) {}
        constexpr TFixed(double v) : Value(ToFixedValue(v)), _Debug(static_cast<double>(Value) / Td) {}

        template <uint32 Ud, class U>
        constexpr TFixed(const TFixed<Ud, U>& other) : Value(ConvertToRaw(other)), _Debug(static_cast<double>(Value) / Td) {}

        constexpr explicit operator int32() const { return (int32)FromFixedValue<double>(Value); }
        constexpr explicit operator uint32() const { return (uint32)FromFixedValue<double>(Value); }
        constexpr explicit operator int64() const { return (int64)FromFixedValue<double>(Value); }
        constexpr explicit operator uint64() const { return (uint64)FromFixedValue<double>(Value); }
        constexpr explicit operator float() const { return FromFixedValue<float>(Value); }
        constexpr explicit operator double() const { return FromFixedValue<double>(Value); }

        template <uint32 Ud, class U>
        static constexpr T ConvertToRaw(const TFixed<Ud, U>& other)
        {
            if constexpr (Td < Ud)
                return T(int64(other.Value) / (Ud / Td));
            else
                return T(int64(other.Value) * (Td / Ud));
        }

        T Value;
#if DEBUG
        double _Debug = 0;
#endif
    };

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

    PHOENIXSIM_API typedef TFixed<1024> Value;
    PHOENIXSIM_API typedef TFixed<16 * 1024> Distance;
    PHOENIXSIM_API typedef TFixed<Distance::D, int64> Distance2;
    PHOENIXSIM_API typedef TFixed<64> Time;
    PHOENIXSIM_API typedef TFixed<1024> Speed;
    PHOENIXSIM_API typedef TFixed<0x10000000 / 45> Angle;

    static_assert(Value::D == 1024);
    static_assert(Value::FracBits == 10);

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

    template<int32 Td, class T>
    constexpr bool operator==(const TFixed<Td, T>& lhs, const TFixed<Td, T>& rhs)
    {
        return lhs.Value == rhs.Value;
    }

    template<int32 Td, class T, int32 Ud, class U>
    constexpr bool operator==(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        auto a = int64(lhs.Value) * Ud;
        auto b = int64(rhs.Value) * Td;
        return a == b;
    }

    template<int32 Td, class T>
    constexpr bool operator==(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs.Value == TFixed<Td, T>::ToFixedValue(rhs);
    }

    template<int32 Td, class T>
    constexpr bool operator==(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) == rhs;
    }

    // operator!=

    template<int32 Td, class T, int32 Ud, class U>
    constexpr bool operator!=(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        return !operator==(lhs, rhs);
    }

    template<int32 Td, class T>
    constexpr bool operator!=(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs.Value != TFixed<Td, T>::ToFixedValue(rhs);
    }

    template<int32 Td, class T>
    constexpr bool operator!=(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) != rhs;
    }

    // operator<

    template<int32 Td, class T, int32 Ud, class U>
    constexpr bool operator<(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        if constexpr (Td > Ud)
            return lhs.Value < rhs.Value * (Td / Ud);
        else
            return lhs.Value * (Ud / Td) < rhs.Value;
    }

    template<int32 Td, class T>
    constexpr bool operator<(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs < TFixed<Td, T>(rhs);
    }

    template<int32 Td, class T>
    constexpr bool operator<(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) < rhs;
    }

    // operator<=

    template<int32 Td, class T, int32 Ud, class U>
    constexpr bool operator<=(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        if constexpr (Td > Ud)
            return lhs.Value <= rhs.Value * (Td / Ud);
        else
            return lhs.Value * (Ud / Td) <= rhs.Value;
    }

    template<int32 Td, class T>
    constexpr bool operator<=(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs <= TFixed<Td, T>(rhs);
    }

    template<int32 Td, class T>
    constexpr bool operator<=(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) <= rhs;
    }

    // operator>

    template<int32 Td, class T, int32 Ud, class U>
    constexpr bool operator>(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        if constexpr (Td > Ud)
            return lhs.Value > rhs.Value * (Td / Ud);
        else
            return lhs.Value * (Ud / Td) > rhs.Value;
    }

    template<int32 Td, class T>
    constexpr bool operator>(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs > TFixed<Td, T>(rhs);
    }

    template<int32 Td, class T>
    constexpr bool operator>(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) > rhs;
    }

    // operator>=

    template<int32 Td, class T, int32 Ud, class U>
    constexpr bool operator>=(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        if constexpr (Td > Ud)
            return lhs.Value >= rhs.Value * (Td / Ud);
        else
            return lhs.Value * (Ud / Td) >= rhs.Value;
    }

    template<int32 Td, class T>
    constexpr bool operator>=(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs >= TFixed<Td, T>(rhs);
    }

    template<int32 Td, class T>
    constexpr bool operator>=(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) >= rhs;
    }

    //
    // Addition
    //

    template <uint64 Td, class T, uint64 Ud, class U>
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

    template <uint64 Td, class T>
    constexpr auto operator+(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs + TFixed<Td, T>(rhs);
    }

    template <uint64 Td, class T>
    constexpr auto operator+(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) + rhs;
    }

    template<int64 Td, class T, int64 Ud, class U>
    constexpr auto& operator+=(TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        lhs = lhs + rhs;
        return lhs;
    }

    template<int64 Td, class T>
    constexpr auto& operator+=(TFixed<Td, T>& lhs, double rhs)
    {
        lhs.Value = lhs.Value + TFixed<Td, T>::ToFixedValue(rhs);
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

    template <uint64 Td, class T, uint64 Ud, class U>
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

    template <uint64 Td, class T>
    constexpr auto operator-(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs - TFixed<Td, T>(rhs);
    }

    template <uint64 Td, class T>
    constexpr auto operator-(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) - rhs;
    }

    template <uint64 Td, class T>
    constexpr TFixed<Td, T> operator-(const TFixed<Td, T>& lhs)
    {
        return TFixedQ_T<T>(-lhs.Value);
    }

    template<int64 Td, class T, int64 Ud, class U>
    constexpr auto& operator-=(TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        lhs = lhs - rhs;
        return lhs;
    }

    template<int64 Td, class T>
    constexpr auto& operator-=(TFixed<Td, T>& lhs, double rhs)
    {
        lhs.Value = lhs.Value - TFixed<Td, T>::ToFixedValue(rhs);
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

    template<int32 Td, class T, int32 Ud, class U>
    constexpr auto operator*(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        auto v = int64(lhs.Value) * rhs.Value;
        v = v >> TFixed<Ud, U>::FracBits;
        return TFixed<Td, int64>(Q64(v));
    }

    template<int32 Td, class T>
    constexpr auto operator*(const TFixed<Td, T>& lhs, double rhs)
    {
        return TFixed<Td, T>(Q64(int64(lhs.Value) * rhs));
    }

    template <uint64 Td, class T>
    constexpr auto operator*(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) * rhs;
    }

    template<int64 Td, class T, int64 Ud, class U>
    constexpr auto& operator*=(TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }

    template<int64 Td, class T>
    constexpr auto& operator*=(TFixed<Td, T>& lhs, double rhs)
    {
        lhs.Value = lhs.Value * TFixed<Td, T>::ToFixedValue(rhs);
        return lhs;
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

    //
    // Division
    //

    template<int32 Td, class T, int32 Ud, class U>
    constexpr auto operator/(const TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        return TFixed<Td, T>(Q64(int64(lhs.Value) * Ud / rhs.Value));
    }

    template <uint32 Td, class T>
    constexpr auto operator/(const TFixed<Td, T>& lhs, double rhs)
    {
        return lhs / TFixed<Td, T>(rhs);
    }

    template <uint32 Td, class T>
    constexpr auto operator/(double lhs, const TFixed<Td, T>& rhs)
    {
        return TFixed<Td, T>(lhs) / rhs;
    }

    template<int64 Td, class T, int64 Ud, class U>
    constexpr auto& operator/=(TFixed<Td, T>& lhs, const TFixed<Ud, U>& rhs)
    {
        lhs = lhs / rhs;
        return lhs;
    }

    template<int64 Td, class T>
    constexpr auto& operator/=(TFixed<Td, T>& lhs, double rhs)
    {
        lhs = lhs / rhs;
        return lhs;
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
