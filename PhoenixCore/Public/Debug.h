
#pragma once

#include "FixedPoint/FixedVector.h"

namespace Phoenix
{
    struct Color;

    struct IDebugState
    {
        virtual ~IDebugState() = default;

        virtual bool KeyDown(uint32 keycode) const = 0;
        virtual bool KeyUp(uint32 keycode) const = 0;

        virtual bool MouseDown(uint32 keycode) const = 0;
        virtual bool MouseUp(uint32 keycode) const = 0;

        virtual Vec2 GetMousePos() const = 0;
    };

    struct IDebugRenderer
    {
        virtual ~IDebugRenderer() = default;

        virtual void DrawCircle(const Vec2& pt, Distance radius, const Color& color, int32 segments = 32) = 0;
        virtual void DrawLine(const Vec2& v0, const Vec2& v1, const Color& color) = 0;
        virtual void DrawLine(const Line2& line, const Color& color) = 0;
        virtual void DrawDebugText(const Vec2& pt, const char* str, size_t len, const Color& color) = 0;

        virtual Color GetColor(uint32 index) const = 0;
    };
}
