
#include "SDLDebugRenderer.h"

#include <vector>
#include <SDL3/SDL_render.h>

#include "Color.h"

using namespace Phoenix;

namespace SDLDebugRenderer_Private
{
    void SDL_RenderCircle(SDL_Renderer *renderer, float x1, float y1, float radius, int32 segments = 32)
    {
        std::vector<SDL_FPoint> points;
        points.reserve(segments);

        for (int32 i = 0; i < segments - 1; ++i)
        {
            float angle = float(i) * (2.0f * 3.14f) / (segments - 1);
            float x = x1 + cos(angle) * radius; 
            float y = y1 + sin(angle) * radius;
            points.emplace_back(x, y);
        }

        points.push_back(points[0]);
    
        SDL_RenderLines(renderer, points.data(), segments);
    }
}

SDLDebugRenderer::SDLDebugRenderer(SDL_Renderer* renderer)
    : Renderer(renderer)
{
    for (Color& color : Colors)
    {
        color = Color(rand() % 255, rand() % 255, rand() % 255);
    }
}

void SDLDebugRenderer::DrawCircle(const Vec2& pt, Distance radius, const Color& color, int32 segments)
{
    auto ptx = MapCenter.X + pt.X;
    auto pty = MapCenter.Y - pt.Y;
    SDL_SetRenderDrawColor(Renderer, color.R, color.G, color.B, SDL_ALPHA_OPAQUE);
    SDLDebugRenderer_Private::SDL_RenderCircle(Renderer, (float)ptx, (float)pty, (float)radius, segments);
}

void SDLDebugRenderer::DrawLine(const Vec2& v0, const Vec2& v1, const Color& color)
{
    auto pt0x = MapCenter.X + v0.X;
    auto pt0y = MapCenter.Y - v0.Y;
    auto pt1x = MapCenter.X + v1.X;
    auto pt1y = MapCenter.Y - v1.Y;
    SDL_SetRenderDrawColor(Renderer, color.R, color.G, color.B, SDL_ALPHA_OPAQUE);
    SDL_RenderLine(Renderer, (float)pt0x, (float)pt0y, (float)pt1x, (float)pt1y);
}

void SDLDebugRenderer::DrawLine(const Line2& line, const Color& color)
{
    DrawLine(line.Start, line.End, color);
}

void SDLDebugRenderer::DrawDebugText(const Vec2& pt, const char* str, size_t len, const Color& color)
{
    auto ptx = MapCenter.X + pt.X;
    auto pty = MapCenter.Y - pt.Y;
    SDL_SetRenderDrawColor(Renderer, color.R, color.G, color.B, SDL_ALPHA_OPAQUE);
    SDL_RenderDebugText(Renderer, (float)ptx, (float)pty, str);
}

Color SDLDebugRenderer::GetColor(uint32 index) const
{
    return Colors[index % _countof(Colors)];
}
