
#pragma once

#include <cstdlib> // for rand(), remove once we have a deterministic rand

#include "FixedMath.h"

namespace Phoenix
{
    template <class T = Fixed32_8>
    struct TVec2
    {
        using ComponentT = T;

        static const TVec2 Zero;
        static const TVec2 One;
        static const TVec2 XAxis;
        static const TVec2 YAxis;
        static const TVec2 Min;
        static const TVec2 Max;

        constexpr TVec2() = default;
        constexpr TVec2(T xy) : X(xy), Y(xy) {}
        constexpr TVec2(T x, T y) : X(x), Y(y) {}
        constexpr TVec2(const Cordic::TCordicVec<T, T>& vec) : X(vec.X), Y(vec.Y) {}

        constexpr TVec2 operator+(const T& rhs) const
        {
            return { X + rhs, Y + rhs };
        }

        constexpr TVec2 operator+(const TVec2& rhs) const
        {
            return { X + rhs.X, Y + rhs.Y };
        }

        TVec2& operator+=(const T& rhs)
        {
            X += rhs;
            Y += rhs;
            return *this;
        }

        TVec2& operator+=(const TVec2& rhs)
        {
            X += rhs.X;
            Y += rhs.Y;
            return *this;
        }

        constexpr TVec2 operator-(const T& rhs) const
        {
            return { X - rhs, Y - rhs };
        }

        constexpr TVec2 operator-(const TVec2& rhs) const
        {
            return { X - rhs.X, Y - rhs.Y };
        }

        TVec2& operator-=(const T& rhs)
        {
            X -= rhs;
            Y -= rhs;
            return *this;
        }

        TVec2& operator-=(const TVec2& rhs)
        {
            X -= rhs.X;
            Y -= rhs.Y;
            return *this;
        }

        constexpr TVec2 operator*(const TVec2& rhs) const
        {
            return { X * rhs.X, Y * rhs.Y }; 
        }

        template <class U = Fixed32_12>
        constexpr TVec2 operator*(const U& rhs) const
        {
            return { X * rhs, Y * rhs }; 
        }

        template <class U = Fixed32_12>
        friend TVec2 operator*(const U& lhs, const TVec2& rhs)
        {
            return { lhs * rhs.X, lhs * rhs.Y }; 
        }

        TVec2& operator*=(const TVec2& rhs)
        {
            X *= rhs.X;
            Y *= rhs.Y;
            return *this; 
        }

        template <class U>
        TVec2& operator*=(const U& rhs)
        {
            X *= rhs;
            Y *= rhs;
            return *this; 
        }

        constexpr TVec2 operator/(const TVec2& rhs) const
        {
            return { X / rhs.X, Y / rhs.Y }; 
        }

        template <class U>
        constexpr TVec2 operator/(const U& rhs) const
        {
            return { X / rhs, Y / rhs }; 
        }

        template <class U = Fixed32_12>
        friend TVec2 operator/(const U& lhs, const TVec2& rhs)
        {
            return { lhs / rhs.X, lhs / rhs.Y }; 
        }

        TVec2& operator/=(const TVec2& rhs)
        {
            X /= rhs.X;
            Y /= rhs.Y;
            return *this; 
        }

        template <class U = Fixed32_12>
        TVec2& operator/=(const T& rhs)
        {
            X /= rhs;
            Y /= rhs;
            return *this; 
        }

        constexpr TVec2& operator-()
        {
            X = -X;
            Y = -Y;
            return *this;
        }

        constexpr TVec2 operator-() const
        {
            return { X * -1.0f, Y * -1.0f };
        }

        constexpr Angle AsRadians() const
        {
            return ToAngle(X, Y);
        }

        constexpr Angle AsDegrees() const
        {
            return Rad2Deg(AsRadians());
        }

        constexpr T Length() const
        {
            return Magnitude(X, Y);
        }

        constexpr TVec2 Normalized() const
        {
            /// make a change
            auto m = Length();
            TVec2 result = *this;
            return m == 0.0f ? result : result / m;
        }

        constexpr TVec2 Rotate(Angle angle) const
        {
            return Cordic::Rotate(X, Y, angle);
        }

        constexpr static bool Equals(const TVec2& a, const TVec2& b, T threshold = T::Epsilon)
        {
            return Abs(a.X - b.X) < threshold && Abs(a.Y - b.Y) < threshold;
        }

        constexpr static auto Dot(const TVec2& a, const TVec2& b)
        {
            return Cordic::Dot(a.X, a.Y, b.X, b.Y);
        }

        constexpr static auto Distance(const TVec2& a, const TVec2& b)
        {
            return Magnitude(a.X - b.X, a.Y - b.Y);
        }

        constexpr static TVec2 Project(const TVec2& s, const TVec2& n, const TVec2& p)
        {
            T a = (p.X - s.X) * n.X + (p.Y - s.Y) * n.Y;
            T b = n.X * n.X + n.Y * n.Y;
            T d = a / b;
            return { p.X - d * n.X, p.Y - d * n.Y };
        }

        constexpr static TVec2 Reflect(const TVec2& n, const TVec2& v)
        {
            return v - 2 * (Dot(v, n) / Dot(n, n)) * n;
        }

        constexpr static auto Cross(const TVec2& a, const TVec2& b)
        {
            return a.X*b.Y - a.Y*b.X;
        }

        static TVec2 RandUnitVector()
        {
            Angle deg = static_cast<float>(rand() % 1440) / 4.0f;
            return XAxis.Rotate(deg);
        }

        constexpr static auto Intersects(
            const TVec2& a0,
            const TVec2& a1,
            const TVec2& b0,
            const TVec2& b1,
            TVec2& outPt)
        {
            if ((a0.X == a1.X && a0.Y == a1.Y) || (b0.X == b1.X && b0.Y == b1.Y)) 
            {
                return false;
            }

            auto d = (b1.Y - b0.Y) * (a1.X - a0.X) - (b1.X - b0.X) * (a1.Y - a0.Y);
            if (d == 0)
            {
                return false;
            }

            auto dx = a0.X - b0.X;
            auto dy = a0.Y - b0.Y;

            auto ua = ((b1.X - b0.X) * dy - (b1.Y - b0.Y) * dx);
            auto ub = ((a1.X - a0.X) * dy - (a1.Y - a0.Y) * dx);

            ua = ua / d;
            ub = ub / d;

            if (ua < 0 || ua > 1 || ub < 0 || ub > 1)
            {
                return false;
            }

            outPt.X = a0.X + ua * (a1.X - a0.X);
            outPt.Y = a0.Y + ua * (a1.Y - a0.Y);
            return true;
        }

        constexpr static TVec2 Midpoint(const TVec2& a, const TVec2& b)
        {
            return (a + b) / 2.0;
        }

        constexpr static TVec2 FromPolar(Angle a, T r)
        {
            return Cordic::Rotate<T>(r, 0, a);
        }

        T X = 0;
        T Y = 0;
    };

    using Vec2 = TVec2<Distance>;

    template <class T> const TVec2<T> TVec2<T>::Zero = { 0, 0 };
    template <class T> const TVec2<T> TVec2<T>::One = { 1, 1 };
    template <class T> const TVec2<T> TVec2<T>::XAxis = { 1, 0 };
    template <class T> const TVec2<T> TVec2<T>::YAxis = { 0, 1 };
    template <class T> const TVec2<T> TVec2<T>::Min = { T::Min, T::Min };
    template <class T> const TVec2<T> TVec2<T>::Max = { T::Max, T::Max };

    template <class> struct TZero {};
    template <uint32 Td, class T> struct TZero<TFixed<Td, T>> { static constexpr TFixed<Td, T> Value = 0; };
    template <class T> struct TZero<TVec2<T>> { static constexpr TVec2<T> Value = TVec2<T>(0, 0); };

    template <class> struct TOne {};
    template <uint32 Td, class T> struct TOne<TFixed<Td, T>> { static constexpr TFixed<Td, T> Value = 1; };
    template <class T> struct TOne<TVec2<T>> { static constexpr TVec2<T> Value = TVec2<T>(1, 1); };
}