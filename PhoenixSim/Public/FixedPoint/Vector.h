
#pragma once

#include "FixedMath.h"
#include "FixedPoint/FixedPoint.h"

namespace Phoenix
{
    struct PHOENIXSIM_API Vec2
    {
        static const Vec2 Zero;
        static const Vec2 One;
        static const Vec2 XAxis;
        static const Vec2 YAxis;

        constexpr Vec2() = default;
        constexpr Vec2(Distance x, Distance y) : X(x), Y(y) {}

        constexpr Vec2 operator+(Distance rhs) const
        {
            return { X + rhs, Y + rhs };
        }

        constexpr Vec2 operator+(const Vec2& rhs) const
        {
            return { X + rhs.X, Y + rhs.Y };
        }

        Vec2& operator+=(Distance rhs)
        {
            X += rhs;
            Y += rhs;
            return *this;
        }

        Vec2& operator+=(const Vec2& rhs)
        {
            X += rhs.X;
            Y += rhs.Y;
            return *this;
        }

        constexpr Vec2 operator-(Distance rhs) const
        {
            return { X - rhs, Y - rhs };
        }

        constexpr Vec2 operator-(const Vec2& rhs) const
        {
            return { X - rhs.X, Y - rhs.Y };
        }

        Vec2& operator-=(Distance rhs)
        {
            X -= rhs;
            Y -= rhs;
            return *this;
        }

        Vec2& operator-=(const Vec2& rhs)
        {
            X -= rhs.X;
            Y -= rhs.Y;
            return *this;
        }

        constexpr Vec2 operator*(const Vec2& rhs) const
        {
            return { X * rhs.X, Y * rhs.Y }; 
        }

        constexpr Vec2 operator*(Value rhs) const
        {
            return { X * rhs, Y * rhs }; 
        }

        friend Vec2 operator*(Value lhs, const Vec2& rhs)
        {
            return { lhs * rhs.X, lhs * rhs.Y }; 
        }

        Vec2& operator*=(const Vec2& rhs)
        {
            X *= rhs.X;
            Y *= rhs.Y;
            return *this; 
        }

        Vec2& operator*=(Value rhs)
        {
            X *= rhs;
            Y *= rhs;
            return *this; 
        }

        constexpr Vec2 operator/(const Vec2& rhs) const
        {
            return { X / rhs.X, Y / rhs.Y }; 
        }

        constexpr Vec2 operator/(Value rhs) const
        {
            return { X / rhs, Y / rhs }; 
        }

        Vec2& operator/=(const Vec2& rhs)
        {
            X /= rhs.X;
            Y /= rhs.Y;
            return *this; 
        }

        Vec2& operator/=(Value rhs)
        {
            X /= rhs;
            Y /= rhs;
            return *this; 
        }

        constexpr Vec2& operator-()
        {
            X = -X;
            Y = -Y;
            return *this;
        }

        constexpr Vec2 operator-() const
        {
            return { X * -1.0f, Y * -1.0f };
        }

        constexpr Angle AsDegrees() const
        {
            return FixedMath::Rad2Deg(FixedMath::Atan2(Y, X));
        }

        constexpr Distance Length() const
        {
            auto a = LengthSq();
            return FixedMath::Sqrt(a);
        }

        constexpr Distance2 LengthSq() const
        {
            Distance2 x = X * X;
            Distance2 y = Y * Y;
            return x + y;
        }

        constexpr Vec2 Normalized() const
        {
            Value m = Length();
            Vec2 result = *this;
            return m == 0.0f ? result : result / m;
        }

        constexpr Vec2 Rotate(Angle degrees) const
        {
            Angle theta = FixedMath::Deg2Rad(degrees);
            auto cs = FixedMath::Cos(theta);
            auto sn = FixedMath::Sin(theta);
            Phoenix::Distance x = X * cs;
            x -= Y * sn;
            Phoenix::Distance y = X * sn;
            y += Y * cs;
            return { x, y };
            // return { X * cs - Y * sn, X * sn + Y * cs };
        }

        constexpr static bool Equals(const Vec2& a, const Vec2& b, Distance threshold = 1E-3f)
        {
            return FixedMath::Abs(a.X - b.X) < threshold && FixedMath::Abs(a.Y - b.Y) < threshold;
        }

        constexpr static Value Dot(const Vec2& a, const Vec2& b)
        {
            auto a1 = a.X * b.X;
            auto a2 = a.Y * b.Y;
            return a1 + a2;
        }

        constexpr static Distance DistanceSq(const Vec2& a, const Vec2& b)
        {
            return (a.X - b.X) * (a.X - b.X) + (a.Y - b.Y) * (a.Y - b.Y);
        }

        constexpr static Distance Distance(const Vec2& a, const Vec2& b)
        {
            return FixedMath::Sqrt(DistanceSq(a, b));
        }

        constexpr static Vec2 Project(const Vec2& s, const Vec2& n, const Vec2& p)
        {
            Phoenix::Distance a = (p.X - s.X) * n.X + (p.Y - s.Y) * n.Y;
            Phoenix::Distance b = n.X * n.X + n.Y * n.Y;
            Phoenix::Distance d = a / b;
            return { p.X - d * n.X, p.Y - d * n.Y };
        }

        constexpr static Vec2 Reflect(const Vec2& n, const Vec2& v)
        {
            return v - 2 * (Dot(v, n) / Dot(n, n)) * n;
        }

        constexpr static Value Cross(const Vec2& a, const Vec2& b)
        {
            return a.X*b.Y - a.Y*b.X;
        }

        static Vec2 RandUnitVector()
        {
            Angle deg = static_cast<float>(rand() % 1440) / 4.0f;
            return XAxis.Rotate(deg);
        }

        Phoenix::Distance X = 0;
        Phoenix::Distance Y = 0;
    };

    struct PHOENIXSIM_API Vec3
    {
        static const Vec3 Zero;
        static const Vec3 One;
        static const Vec3 XAxis;
        static const Vec3 YAxis;
        static const Vec3 ZAxis;

        constexpr Vec3() = default;
        constexpr Vec3(Distance xyz) : X(xyz), Y(xyz), Z(xyz) {}
        constexpr Vec3(Distance x, Distance y, Distance z = 0) : X(x), Y(y), Z(z) {}

        constexpr Vec3 operator+(Distance rhs) const
        {
            return { X + rhs, Y + rhs, Z + rhs };
        }

        constexpr Vec3 operator+(const Vec3& rhs) const
        {
            return { X + rhs.X, Y + rhs.Y, Z + rhs.Z };
        }

        Vec3& operator+=(Distance rhs)
        {
            X += rhs;
            Y += rhs;
            Z += rhs;
            return *this;
        }

        Vec3& operator+=(const Vec3& rhs)
        {
            X += rhs.X;
            Y += rhs.Y;
            Z += rhs.Z;
            return *this;
        }

        constexpr Vec3 operator-(Distance rhs) const
        {
            return { X + rhs, Y + rhs, Z + rhs };
        }

        constexpr Vec3 operator-(const Vec3& rhs) const
        {
            return { X + rhs.X, Y + rhs.Y, Z + rhs.Z };
        }

        Vec3& operator-=(Distance rhs)
        {
            X -= rhs;
            Y -= rhs;
            Z -= rhs;
            return *this;
        }

        Vec3& operator-=(const Vec3& rhs)
        {
            X -= rhs.X;
            Y -= rhs.Y;
            Z -= rhs.Z;
            return *this;
        }
        
        Distance X = 0;
        Distance Y = 0;
        Distance Z = 0;
    };

    // Represents a line between 2 points.
    struct PHOENIXSIM_API Line2
    {
        constexpr Line2() = default;
        constexpr Line2(const Vec2& start, const Vec2& end) : Start(start), End(end) {}
        
        Vec2 Start;
        Vec2 End;

        // Linearly interpolates between Start and End by the parameter t.
        constexpr Vec2 Lerp(Value t)
        {
            return Start + (End - Start) * t;
        }

        // Returns the vector between Start and End.
        constexpr Vec2 GetVector() const
        {
            return End - Start;
        }

        // Returns the normalized vector between Start and End.
        constexpr Vec2 GetDirection() const
        {
            return GetVector().Normalized();
        }

        // Returns the vector from a Vec2 point to a Line. 
        constexpr static Vec2 VectorToLine(const Line2& line, const Vec2& point)
        {
            Vec2 a = point - line.Start;
            Vec2 b = line.End - line.Start;
            auto bb = Vec2::Dot(b, b);
            if (bb == 0) return Vec2::Zero;
            Value d = Vec2::Dot(a, b) / bb;
            d = FixedMath::Min(FixedMath::Max(d, Value(0.0f)), Value(1.0f));
            return -(a - b * d);
        }

        // Returns the squared distance from a Vec2 point to a Line. 
        constexpr static Distance DistanceToLineSq(const Line2& line, const Vec2& point)
        {
            return VectorToLine(line, point).LengthSq();
        }

        // Returns the distance from a Vec2 point to a Line.
        constexpr static Distance DistanceToLine(const Line2& line, const Vec2& point)
        {
            return FixedMath::Sqrt(DistanceToLineSq(line, point));
        }
    };

    struct PHOENIXSIM_API Line3
    {
        constexpr Line3() = default;
        constexpr Line3(const Vec3& start, const Vec3& end) : Start(start), End(end) {}

        Vec3 Start;
        Vec3 End;
    };

    template <class> struct TZero {};
    template <uint32 Td, class T> struct TZero<TFixed<Td, T>> { static constexpr TFixed<Td, T> Value = 0; };
    template <> struct TZero<Vec2> { static constexpr Vec2 Value = Vec2(0, 0); };
    template <> struct TZero<Vec3> { static constexpr Vec3 Value = Vec3(0, 0); };

    template <class> struct TOne {};
    template <uint32 Td, class T> struct TOne<TFixed<Td, T>> { static constexpr TFixed<Td, T> Value = 1; };
    template <> struct TOne<Vec2> { static constexpr Vec2 Value = Vec2(1, 1); };
    template <> struct TOne<Vec3> { static constexpr Vec3 Value = Vec3(1, 1); };
}