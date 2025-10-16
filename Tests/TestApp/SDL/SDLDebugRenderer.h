
#pragma once

#include <SDL3/SDL_render.h>

#include "Debug.h"
#include "Color.h"
#include "PhoenixCore.h"

namespace Phoenix
{
    struct SDLViewport;
}

namespace Phoenix
{
    struct SDLDebugRenderer : public IDebugRenderer
    {
        SDLDebugRenderer(SDL_Renderer* renderer, SDLViewport* viewport);

        void Reset();

        void DrawCircle(const Vec2& pt, Distance radius, const Color& color, int32 segments = 32) override;

        void DrawLine(const Vec2& pt0, const Vec2& pt1, const Color& color) override;

        void DrawLine(const Line2& line, const Color& color) override;

        void DrawLines(const Vec2* points, size_t num, const Color& color) override;

        void DrawRect(const Vec2& min, const Vec2& max, const Color& color) override;

        void DrawDebugText(const Vec2& pt, const char* str, size_t len, const Color& color) override;

        float GetScale() const;
        void PushScale(float scale);
        void PopScale();

        Color GetColor(size_t index) const override;

        SDL_Renderer* Renderer;
        SDLViewport* Viewport;
        Color Colors[1024];
        TArray<float> ScaleStack;
    };
}
