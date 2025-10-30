#pragma once

#include <limits>

#include "Platform.h"

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

    // Calculates the number of bits needed to store the value n.
    // https://en.cppreference.com/w/cpp/numeric/bit_width
    template <class T>
    constexpr T BitWidth(T n)
    {
        T un = n;
        T c = 0;
        for (; un != 0; un >>= 1) ++c;
        return static_cast<T>(c) * (n < 0 ? -1 : 1);
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

    template <class TFrom, class TTo>
    static constexpr TTo ConvertTo(TFrom fromValue, uint8 fromB, uint8 toB)
    {
        int64 Td = 1LL << toB;
        int64 Ud = 1LL << fromB;
        int64 val = int64(fromValue) * Td / Ud;
        constexpr TTo qmint = std::numeric_limits<TTo>::min();
        constexpr TTo qmaxt = std::numeric_limits<TTo>::max();
        if (val > (TTo)qmaxt) return (TTo)qmaxt;
        if (val < (TTo)qmint) return (TTo)qmint;
        return TTo(val);
    }

    template <class TFrom, class TTo>
    static constexpr TTo ConvertToQ(TFrom fromValue, uint8 toB)
    {
        int64 Td = 1LL << toB;
        int64 val = int64(fromValue * Td);
        constexpr TTo qmint = std::numeric_limits<TTo>::min();
        constexpr TTo qmaxt = std::numeric_limits<TTo>::max();
        if (val > (TTo)qmaxt) return qmaxt;
        if (val < (TTo)qmint) return qmint;
        return static_cast<TTo>(val);
    }

    template <class TFrom, class TTo>
    static constexpr TTo ConvertFromQ(TFrom fromValue, uint8 fromB)
    {
        int64 Td = 1LL << fromB;
        return static_cast<TTo>(fromValue) / Td;
    }

    template <uint8 Tb, class T = int32>
    struct TFixed
    {
        static_assert(Tb < (sizeof(T) << 3) - 2);
        using ValueT = T;
        using QT = TFixedQ_T<T>;
        static constexpr int64 D = 1LL << Tb;
        static constexpr int32 B = Tb;

        static constexpr TFixedQ_T<T> QMIN = TFixedQ_T<T>(std::numeric_limits<T>::min());
        static constexpr TFixedQ_T<T> QMAX = TFixedQ_T<T>(std::numeric_limits<T>::max());
        static constexpr TFixedQ_T<T> QSTEP = TFixedQ_T<T>(1);
        static constexpr TFixedQ_T<T> QZero = TFixedQ_T<T>(0);
        static constexpr TFixedQ_T<T> QOne = TFixedQ_T<T>(D);

        static const TFixed Epsilon;
        static const TFixed Min;
        static const TFixed Max;
        static const TFixed Step;

        // Convert from U to TFixed<Tb, T>::TValue
        template <class U> static constexpr T ConvertToQ(U u) { return T(Phoenix::ConvertToQ<U, T>(u, Tb)); }

        // Convert from TFixed<Tb, T>::TValue to U
        template <class U> static constexpr U ConvertFromQ(T v) { return U(Phoenix::ConvertFromQ<T, U>(v, Tb)); }

        // Convert from TFixed<Ub, U> to TFixed<Tb, T>::TValue
        template <class U> static constexpr T ConvertTo(U u, uint8 ub)
        {
            return T(Phoenix::ConvertTo<U, T>(u, ub, Tb));
        }

        // Convert from TFixed<Ub, U> to TFixed<Tb, T>::TValue
        template <int32 Ub, class U>
        static constexpr T ConvertTo(const TFixed<Ub, U>& other)
        {
            return T(Phoenix::ConvertTo<U, T>(other.Value, Ub, Tb));
        }

        constexpr TFixed() : Value(0){}
        constexpr TFixed(Q32 v) : Value(static_cast<T>(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(Q64 v) : Value(static_cast<T>(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(int32 v) : Value(ConvertToQ(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(uint32 v) : Value(ConvertToQ(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(int64 v) : Value(ConvertToQ(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(uint64 v) : Value(ConvertToQ(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(float v) : Value(ConvertToQ(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}
        constexpr TFixed(double v) : Value(ConvertToQ(v)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}

        template <int32 Ub, class U>
        constexpr TFixed(const TFixed<Ub, U>& other) : Value(ConvertTo(other)) TFIXED_DEBUG_FIELD(static_cast<double>(Value) / D) {}

        constexpr explicit operator Q32() const { return (Q32)ConvertFromQ<double>(Value); }
        constexpr explicit operator Q64() const { return (Q64)ConvertFromQ<double>(Value); }
        constexpr explicit operator int8() const { return (int8)ConvertFromQ<double>(Value); }
        constexpr explicit operator uint8() const { return (uint8)ConvertFromQ<double>(Value); }
        constexpr explicit operator int16() const { return (int16)ConvertFromQ<double>(Value); }
        constexpr explicit operator uint16() const { return (uint16)ConvertFromQ<double>(Value); }
        constexpr explicit operator int32() const { return (int32)ConvertFromQ<double>(Value); }
        constexpr explicit operator uint32() const { return (uint32)ConvertFromQ<double>(Value); }
        constexpr explicit operator int64() const { return (int64)ConvertFromQ<double>(Value); }
        constexpr explicit operator uint64() const { return (uint64)ConvertFromQ<double>(Value); }
        constexpr explicit operator float() const { return ConvertFromQ<float>(Value); }
        constexpr explicit operator double() const { return ConvertFromQ<double>(Value); }

        T Value;
#if TFIXED_DEBUG
        double _Debug = 0;
#endif
    };

    template <uint8 Tb, class T> const TFixed<Tb, T> TFixed<Tb, T>::Epsilon = TFixed(1E-3);
    template <uint8 Tb, class T> const TFixed<Tb, T> TFixed<Tb, T>::Min = QMIN;
    template <uint8 Tb, class T> const TFixed<Tb, T> TFixed<Tb, T>::Max = QMAX;
    template <uint8 Tb, class T> const TFixed<Tb, T> TFixed<Tb, T>::Step = QSTEP;

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
    template <uint8 Tb, class T = int32>
    struct TInvFixed
    {
        using ValueT = T;
        static constexpr int64 D = 1 << Tb;
        static constexpr int32 B = Tb;
        
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

    struct FP32;
    struct FP64;

    // Generic FixedPoint structure.
    struct FP32
    {
        using TValue = int32;

        constexpr FP32(TValue v = 0, uint8 b = 0) : B(b), Value(v) {}
        constexpr FP32(const FP64& other64);
        constexpr FP32(const FP32& other) : B(other.B), Value(other.Value) {}
        constexpr FP32(FP32&& other) noexcept : B(other.B), Value(other.Value) {}

        // Copy from TFixed<Tb, T>
        template <uint8 Tb, class T>
        constexpr FP32(const TFixed<Tb, T>& v, uint8 b = Tb) : B(b), Value(ConvertTo<T, TValue>(v.Value, Tb, b)) {}

        // Convert to TFixed<Tb, T>
        template <uint8 Tb, class T>
        constexpr operator TFixed<Tb, T>() const { return TFixedQ_T<T>(TFixed<Tb, T>::ConvertTo(Value, B)); }

        // Convert to FP64
        constexpr operator FP64() const;

        FP32& operator=(const FP32& other)
        {
            B = other.B;
            Value = other.Value;
            return *this;
        }

        constexpr bool IsValid() const
        {
            return B < (sizeof(TValue) << 3);
        }

        uint8 B;
        TValue Value;
    };

    // Generic 64-bit FixedPoint structure.
    struct FP64
    {
        using TValue = int64;

        constexpr FP64(TValue v = 0, uint8 b = 0) : B(b), Value(v) {}
        constexpr FP64(const FP32& other32);
        constexpr FP64(const FP64& other) : B(other.B), Value(other.Value) {}
        constexpr FP64(FP64&& other) noexcept : B(other.B), Value(other.Value) {}

        // Copy from TFixed<Tb, T>
        template <uint8 Tb, class T>
        constexpr FP64(const TFixed<Tb, T>& v, uint8 b = Tb) : B(b), Value(ConvertTo<T, TValue>(v.Value, Tb, b)) {}

        // Convert to TFixed<Tb, T>
        template <uint8 Tb, class T>
        constexpr operator TFixed<Tb, T>() const { return TFixedQ_T<T>(TFixed<Tb, T>::ConvertTo(Value, B)); }

        // Convert to FP32
        constexpr operator FP32() const;

        FP64& operator=(const FP64& other)
        {
            B = other.B;
            Value = other.Value;
            return *this;
        }

        constexpr bool IsValid() const
        {
            return B < (sizeof(TValue) << 3);
        }

        uint8 B;
        TValue Value;
    };

    // FP32
    constexpr FP32::FP32(const FP64& other64): B(other64.B), Value(static_cast<TValue>(other64.Value)) {}
    constexpr FP32::operator FP64() const { return { Value, B }; }

    // FP64
    constexpr FP64::FP64(const FP32& other32): B(other32.B), Value(other32.Value) {}
    constexpr FP64::operator FP32() const { return { static_cast<FP32::TValue>(Value), B }; }
}

#include "FixedPoint.inl"