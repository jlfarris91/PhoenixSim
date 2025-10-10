
#pragma once

#include "FixedCordic.h"

namespace Phoenix
{
    template <class T>
    struct TVec2;

    template <uint64 Tb, class T>
    constexpr TFixed<Tb, T> Sqrt(const TFixed<Tb, T>& value)
    {
        return Cordic::Sqrt(value);
    }

    constexpr Distance Cos(Angle angle)
    {
        Distance c, s;
        Cordic::CosSin(angle, c, s);
        return c;
    }

    constexpr Distance Sin(Angle angle)
    {
        Distance c, s;
        Cordic::CosSin(angle, c, s);
        return s;
    }

    constexpr Angle Atan2(const Distance& y, const Distance& x)
    {
        return Cordic::ArcTan2(x, y);
    }

    constexpr Distance Magnitude(Distance x, Distance y)
    {
        return Cordic::Vector(x, y).X;
    }

    constexpr Angle ToAngle(Distance x, Distance y)
    {
        return Cordic::Vector(x, y).Y;
    }

    constexpr Angle AngleBetween(Angle a, Angle b)
    {
        Angle d = Abs(a - b);
        if (d > PI)
        {
            d = TWO_PI - d;
        }
        return d;
    }

    template <class A, class B>
    constexpr auto Min(const A& a, const B& b)
    {
        return a < b ? a : b;
    }

    template <class A, class B>
    constexpr auto Max(const A& a, const B& b)
    {
        return a > b ? a : b;
    }

    template <class A>
    constexpr auto Clamp(const A& value, const A& min, const A& max)
    {
        return value >= max ? max : value <= min ? min : value;
    }

    template <class T>
    constexpr auto Wrap(T value, T minInclusive, T maxExclusive)
    {
        auto d = maxExclusive - minInclusive;
        if (d == 0) return 0;
        while (value >= maxExclusive) value -= d;
        while (value < minInclusive) value += d;
        return value;
    }

    template <class T>
    auto TriangleArea(const TVec2<T>& a, const TVec2<T>& b, const TVec2<T>& c)
    {
        auto av = b - a;
        auto bv = c - a;
        return av.Y * bv.X - av.X * bv.Y;
    }

    template <class T>
    auto Orient(const TVec2<T>& a, const TVec2<T>& b, const TVec2<T>& p)
    {
        return (b.X - a.X) * (p.Y - a.Y) - (b.Y - a.Y) * (p.X - a.X);
    }

    enum class EPointInFaceResult
    {
        Outside,
        Inside,
        OnEdge
    };

    template <class TIdx = uint8>
    struct PointInFaceResult
    {
        EPointInFaceResult Result;
        TIdx OnEdgeIndex = 0;
    };

    // > 0 : inside
    // == 0 : co-circular
    // < 0 : outside
    template <class T = Distance>
    auto PointInCircle(const TVec2<T>& a, const TVec2<T>& b, const TVec2<T>& c, const TVec2<T>& p)
    {
        auto a2 = a.X*a.X + a.Y*a.Y;
        auto b2 = b.X*b.X + b.Y*b.Y;
        auto c2 = c.X*c.X + c.Y*c.Y;

        auto d = 2 * (a.X * (b.Y - c.Y) + b.X * (c.Y - a.Y) + c.X * (a.Y - b.Y));
        if (d == 0)
        {
            return TFixed<T::B, int64>(0);
        }

        auto ux = (a2 * (b.Y - c.Y) + b2 * (c.Y - a.Y) + c2 * (a.Y - b.Y)) / d;
        auto uy = (a2 * (c.X - b.X) + b2 * (a.X - c.X) + c2 * (b.X - a.X)) / d;
        auto r1 = (ux - a.X) * (ux - a.X) + (uy - a.Y) * (uy - a.Y);

        auto v = p - TVec2<T>(ux, uy);
        auto dot = TVec2<T>::Dot(v, v);
        auto dist = r1 - dot;

        return dist;
    }

    template <class T = Distance, class TIdx = uint8>
    PointInFaceResult<TIdx> PointInTriangle(const TVec2<T>& a, const TVec2<T>& b, const TVec2<T>& c, const TVec2<T>& p)
    {
        auto a0 = Orient(a, b, p);
        if (a0 == 0)
        {
            return { EPointInFaceResult::OnEdge, 0 };
        }

        auto b0 = Orient(b, c, p);
        if (b0 == 0)
        {
            return { EPointInFaceResult::OnEdge, 1 };
        }

        auto c0 = Orient(c, a, p);
        if (c0 == 0)
        {
            return { EPointInFaceResult::OnEdge, 2 };
        }

        bool allPos = a0 > 0 && b0 > 0 && c0 > 0;
        bool allNeg = a0 < 0 && b0 < 0 && c0 < 0;
        if (allPos || allNeg)
        {
            return { EPointInFaceResult::Inside };
        }

        return { EPointInFaceResult::Outside };
    }
}
