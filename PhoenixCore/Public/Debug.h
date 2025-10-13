
#pragma once

#include "DLLExport.h"
#include "FixedPoint/FixedVector.h"

namespace Phoenix
{
    struct Color;

    struct PHOENIXCORE_API IDebugState
    {
        virtual ~IDebugState();

        virtual bool KeyDown(uint32 keycode) const = 0;
        virtual bool KeyUp(uint32 keycode) const = 0;

        virtual bool MouseDown(uint8 keycode) const = 0;
        virtual bool MouseUp(uint8 keycode) const = 0;

        virtual Vec2 GetWorldMousePos() const = 0;
    };

    struct PHOENIXCORE_API IDebugRenderer
    {
        virtual ~IDebugRenderer();

        virtual void DrawCircle(const Vec2& pt, Distance radius, const Color& color, int32 segments = 32) = 0;
        virtual void DrawLine(const Vec2& v0, const Vec2& v1, const Color& color) = 0;
        virtual void DrawLine(const Line2& line, const Color& color) = 0;
        virtual void DrawLines(const Vec2* points, size_t num, const Color& color) = 0;
        virtual void DrawRect(const Vec2& min, const Vec2& max, const Color& color) = 0;
        virtual void DrawDebugText(const Vec2& pt, const char* str, size_t len, const Color& color) = 0;

        virtual Color GetColor(uint32 index) const = 0;
    };
}
