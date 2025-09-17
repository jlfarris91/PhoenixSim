#pragma once

#include <cmath>

#include "DLLExport.h"

namespace Phoenix
{
    // TODO (jfarris): implement fixed point someday

    PHOENIXSIM_API typedef float Value;
    PHOENIXSIM_API typedef float Distance;
    PHOENIXSIM_API typedef float Speed;
    PHOENIXSIM_API typedef float Mass;
    PHOENIXSIM_API typedef float Degrees;

    struct PHOENIXSIM_API Vec2
    {
        static const Vec2 Zero;
        static const Vec2 One;
        static const Vec2 XAxis;
        static const Vec2 YAxis;

        Vec2() = default;
        Vec2(Distance xy) : X(xy), Y(xy) {}
        Vec2(Distance x, Distance y) : X(x), Y(y) {}

        Vec2 operator+(Distance rhs) const
        {
            return { X + rhs, Y + rhs };
        }

        Vec2 operator+(const Vec2& rhs) const
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

        Vec2 operator-(Distance rhs) const
        {
            return { X - rhs, Y - rhs };
        }

        Vec2 operator-(const Vec2& rhs) const
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

        Vec2 operator*(const Vec2& rhs) const
        {
            return { X * rhs.X, Y * rhs.Y }; 
        }

        Vec2 operator*(Value rhs) const
        {
            return { X * rhs, Y * rhs }; 
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

        Vec2 operator/(const Vec2& rhs) const
        {
            return { X / rhs.X, Y / rhs.Y }; 
        }

        Vec2 operator/(Value rhs) const
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

        Degrees AsDegrees() const
        {
            return atan2(Y, X) * (180.0f / 3.14f);
        }

        Distance Length() const
        {
            return sqrtf(LengthSq());
        }

        Distance LengthSq() const
        {
            return X * X + Y * Y;
        }

        Vec2 Normalized() const
        {
            Value m = Length();
            Vec2 result = *this;
            return m == 0 ? result : result / m;
        }

        Vec2 Rotate(Degrees degrees) const
        {
            float theta = degrees * (3.14f / 180.0f);
            float cs = cos(theta);
            float sn = sin(theta);
            return { X * cs - Y * sn, X * sn + Y * cs };
        }

        static bool Equals(const Vec2& a, const Vec2& b, Distance threshold = 1E-3f)
        {
            return abs(a.X - b.X) < threshold && abs(a.Y - b.Y) < threshold;
        }

        static float Dot(const Vec2& a, const Vec2& b)
        {
            return a.X * b.X + a.Y * b.Y;
        }

        static Vec2 RandUnitVector()
        {
            Degrees deg = static_cast<float>(rand() % 1440) / 4.0f;
            return XAxis.Rotate(deg);
        }

        Distance X = 0;
        Distance Y = 0;
    };

    struct PHOENIXSIM_API Vec3
    {
        static const Vec3 Zero;
        static const Vec3 One;
        static const Vec3 XAxis;
        static const Vec3 YAxis;
        static const Vec3 ZAxis;

        Vec3() = default;
        Vec3(Distance xyz) : X(xyz), Y(xyz), Z(xyz) {}
        Vec3(Distance x, Distance y, Distance z = 0) : X(x), Y(y), Z(z) {}

        Vec3 operator+(Distance rhs) const
        {
            return { X + rhs, Y + rhs, Z + rhs };
        }

        Vec3 operator+(const Vec3& rhs) const
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

        Vec3 operator-(Distance rhs) const
        {
            return { X + rhs, Y + rhs, Z + rhs };
        }

        Vec3 operator-(const Vec3& rhs) const
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

    template <class T> T Zero();
    template <> inline Value Zero() { return 0; }
    template <> inline Vec2 Zero() { return Vec2::Zero; }
    template <> inline Vec3 Zero() { return Vec3::Zero; }

    template <class T> T One();
    template <> inline Value One() { return 1; }
    template <> inline Vec2 One() { return Vec2::One; }
    template <> inline Vec3 One() { return Vec3::One; }

    template <class TVec, class TRot, class TScale>
    struct PHOENIXSIM_API Transform
    {
        TVec Position = Zero<TVec>();
        TRot Rotation = Zero<TRot>();
        TScale Scale = One<TScale>();
        TVec RotateVector(const TVec& vec);
    };

    typedef Transform<Vec2, Degrees, Value> Transform2D;
    typedef Transform<Vec3, Vec3, Vec3> Transform3D;

    template <>
    inline Vec2 Transform<Vec2, Degrees, Value>::RotateVector(const Vec2& vec)
    {
        return vec.Rotate(Rotation);
    }
}
