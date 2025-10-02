
#pragma once

#include "FixedCordic.h"

namespace Phoenix
{
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

    template <class T>
    constexpr auto Wrap(T value, T minInclusive, T maxExclusive)
    {
        auto d = maxExclusive - minInclusive;
        if (d == 0) return 0;
        while (value >= maxExclusive) value -= d;
        while (value < minInclusive) value += d;
        return value;
    }
}
