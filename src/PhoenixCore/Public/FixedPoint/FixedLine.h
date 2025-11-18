
#pragma once

#include "FixedVector.h"

namespace Phoenix
{
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
    };

    using Line2 = TLine<Vec2>;
}