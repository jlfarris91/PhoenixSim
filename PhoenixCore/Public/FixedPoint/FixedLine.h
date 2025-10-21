
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

        // Returns the distance from a TVec2 point to a Line.
        constexpr static auto DistanceToLine(const TLine& line, const T& point)
        {
            return VectorToLine(line, point).Length();
        }

        constexpr static auto Intersects(const TLine& a, const TLine& b, T& outPt)
        {
            return T::Intersects(a.Start, a.End, b.Start, b.End, outPt);
        }
    };

    using Line2 = TLine<Vec2>;
}