
#pragma once

#include <SDL3/SDL_render.h>

#include "Debug.h"
#include "Color.h"

namespace Phoenix
{
    struct SDLDebugRenderer : public IDebugRenderer
    {
        SDLDebugRenderer(SDL_Renderer* renderer);

        void DrawCircle(const Vec2& pt, Distance radius, const Color& color, int32 segments = 32) override;

        void DrawLine(const Vec2& v0, const Vec2& v1, const Color& color) override;

        void DrawLine(const Line2& line, const Color& color) override;

        void DrawDebugText(const Vec2& pt, const char* str, size_t len, const Color& color) override;

        Color GetColor(uint32 index) const override;

        SDL_Renderer* Renderer;
        Vec2 MapCenter;
        Color Colors[1024];
    };
}
