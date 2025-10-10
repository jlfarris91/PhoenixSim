
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
        constexpr Color(uint8 rgb) : R(rgb), G(rgb), B(rgb), A(255) {}
        constexpr Color(uint8 r, uint8 g, uint8 b, uint8 a = 255) : R(r), G(g), B(b), A(a) {}
        constexpr Color(const Color& other) : R(other.R), G(other.G), B(other.B), A(other.A) {}

        uint8 R = 0;
        uint8 G = 0;
        uint8 B = 0;
        uint8 A = 255;

        Color operator*(Value v) const
        {
            return Color(uint8(R * v), uint8(G * v), uint8(B * v), uint8(A * v));
        }

        Color& operator*=(Value v)
        {
            R = (uint8)(R * v);
            G = (uint8)(G * v);
            B = (uint8)(B * v);
            A = (uint8)(A * v);
            return *this;
        }

        Color operator/(Value v) const
        {
            return Color(uint8(R / v), uint8(G / v), uint8(B / v), uint8(A / v));
        }

        Color& operator/=(Value v)
        {
            R = uint8(R / v); 
            G = uint8(G / v); 
            B = uint8(B / v);
            A = uint8(A / v);
            return *this;
        }
    };
}
