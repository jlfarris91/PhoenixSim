
#pragma once

#include "PlatformTypes.h"
#include "FixedPoint/FixedTypes.h"

namespace Phoenix
{
    struct Color
    {
        static const Color White;
        static const Color Red;
        static const Color Green;
        static const Color Blue;

        constexpr Color() = default;
        constexpr Color(uint8 rgb) : R(rgb), G(rgb), B(rgb) {}
        constexpr Color(uint8 r, uint8 g, uint8 b) : R(r), G(g), B(b) {}
        constexpr Color(const Color& other) : R(other.R), G(other.G), B(other.B) {}

        uint8 R = 0;
        uint8 G = 0;
        uint8 B = 0;

        Color operator/(Value v) const
        {
            return Color(uint32(R / v), uint32(G / v), uint32(B / v));
        }
    };
}
