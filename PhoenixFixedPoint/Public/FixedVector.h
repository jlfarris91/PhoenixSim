
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

        constexpr TVec2() = default;
        constexpr TVec2(T x, T y) : X(x), Y(y) {}

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
            auto v = Cordic::Rotate(X, Y, angle);
            return { v.X, v.Y };
        }

        constexpr static bool Equals(const TVec2& a, const TVec2& b, T threshold = T::Epsilon)
        {
            return Abs(a.X - b.X) < threshold && Abs(a.Y - b.Y) < threshold;
        }

        constexpr static Value Dot(const TVec2& a, const TVec2& b)
        {
            return Cordic::Dot(a.X, a.Y, b.X, b.Y);
        }

        constexpr static T Distance(const TVec2& a, const TVec2& b)
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

        constexpr static Value Cross(const TVec2& a, const TVec2& b)
        {
            return a.X*b.Y - a.Y*b.X;
        }

        static TVec2 RandUnitVector()
        {
            Angle deg = static_cast<float>(rand() % 1440) / 4.0f;
            return XAxis.Rotate(deg);
        }

        T X = 0;
        T Y = 0;
    };

    using Vec2 = TVec2<Distance>;

    template <class T> const TVec2<T> TVec2<T>::Zero = { 0, 0 };
    template <class T> const TVec2<T> TVec2<T>::One = { 1, 1 };
    template <class T> const TVec2<T> TVec2<T>::XAxis = { 1, 0 };
    template <class T> const TVec2<T> TVec2<T>::YAxis = { 0, 1 };

    // Represents a line between 2 points.
    template <class T = TVec2<>>
    struct TLine
    {
        constexpr TLine() = default;
        constexpr TLine(const T& start, const T& end) : Start(start), End(end) {}
        
        T Start;
        T End;

        // Linearly interpolates between Start and End by the parameter t.
        constexpr T Lerp(Value t)
        {
            return Start + (End - Start) * t;
        }

        // Returns the vector between Start and End.
        constexpr T GetVector() const
        {
            return End - Start;
        }

        // Returns the normalized vector between Start and End.
        constexpr T GetDirection() const
        {
            return GetVector().Normalized();
        }

        // Returns the vector from a TVec2 point to a Line. 
        constexpr static T VectorToLine(const TLine& line, const T& point)
        {
            T a = point - line.Start;
            T b = line.End - line.Start;
            auto bb = T::Dot(b, b);
            if (bb == 0) return T::Zero;
            auto d = T::Dot(a, b) / bb;
            d = Min(Max(d, 0.0f), 1.0f);
            return -(a - b * d);
        }

        // Returns the squared distance from a TVec2 point to a Line. 
        constexpr static T DistanceToLineSq(const TLine& line, const T& point)
        {
            return VectorToLine(line, point).LengthSq();
        }

        // Returns the distance from a TVec2 point to a Line.
        constexpr static T DistanceToLine(const TLine& line, const T& point)
        {
            return Sqrt(DistanceToLineSq(line, point));
        }
    };

    using Line2 = TLine<Vec2>;

    template <class> struct TZero {};
    template <uint32 Td, class T> struct TZero<TFixed<Td, T>> { static constexpr TFixed<Td, T> Value = 0; };
    template <class T> struct TZero<TVec2<T>> { static constexpr TVec2<T> Value = TVec2<T>(0, 0); };

    template <class> struct TOne {};
    template <uint32 Td, class T> struct TOne<TFixed<Td, T>> { static constexpr TFixed<Td, T> Value = 1; };
    template <class T> struct TOne<TVec2<T>> { static constexpr TVec2<T> Value = TVec2<T>(1, 1); };
}