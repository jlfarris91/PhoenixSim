
#include "SDLDebugRenderer.h"

#include <vector>
#include <SDL3/SDL_render.h>

#include "Color.h"
#include "SDLCamera.h"
#include "SDLViewport.h"

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

SDLDebugRenderer::SDLDebugRenderer(SDL_Renderer* renderer, SDLViewport* viewport)
    : Renderer(renderer)
    , Viewport(viewport)
{
    for (Color& color : Colors)
    {
        color = Color(rand() % 255, rand() % 255, rand() % 255);
    }
}

void SDLDebugRenderer::Reset()
{
    ScaleStack.clear();
    // PushScale(Viewport->Camera->Zoom);
}

void SDLDebugRenderer::DrawCircle(const Vec2& pt, Distance radius, const Color& color, int32 segments)
{
    SDL_FPoint sdlPt = Viewport->WorldPosToViewportPos(pt);
    SDL_FPoint sdlRadius = Viewport->WorldVecToViewportVec(Vec2(radius, 0));
    SDL_SetRenderDrawColor(Renderer, color.R, color.G, color.B, SDL_ALPHA_OPAQUE);
    SDLDebugRenderer_Private::SDL_RenderCircle(Renderer, sdlPt.x, sdlPt.y, sdlRadius.x, segments);
}

void SDLDebugRenderer::DrawLine(const Vec2& pt0, const Vec2& pt1, const Color& color)
{
    SDL_FPoint sdlPt0 = Viewport->WorldPosToViewportPos(pt0);
    SDL_FPoint sdlPt1 = Viewport->WorldPosToViewportPos(pt1);
    SDL_SetRenderDrawColor(Renderer, color.R, color.G, color.B, SDL_ALPHA_OPAQUE);
    SDL_RenderLine(Renderer, sdlPt0.x, sdlPt0.y, sdlPt1.x, sdlPt1.y);
}

void SDLDebugRenderer::DrawLine(const Line2& line, const Color& color)
{
    DrawLine(line.Start, line.End, color);
}

void SDLDebugRenderer::DrawLines(const Vec2* points, size_t num, const Color& color)
{
    for (size_t i = 0; i < num; ++i)
    {
        DrawLine(points[i], points[(i + 1) % num], color);
    }
}

void SDLDebugRenderer::DrawDebugText(const Vec2& pt, const char* str, size_t len, const Color& color)
{
    SDL_FPoint sdlPt = Viewport->WorldPosToViewportPos(pt);
    SDL_SetRenderDrawColor(Renderer, color.R, color.G, color.B, SDL_ALPHA_OPAQUE);
    SDL_RenderDebugText(Renderer, sdlPt.x, sdlPt.y, str);
}

void SDLDebugRenderer::PushScale(float scale)
{
    ScaleStack.push_back(scale);
    SDL_SetRenderScale(Renderer, scale, scale);
}

void SDLDebugRenderer::PopScale()
{
    float scale = ScaleStack.back();
    ScaleStack.pop_back();
    SDL_SetRenderScale(Renderer, scale, scale);
}

Color SDLDebugRenderer::GetColor(uint32 index) const
{
    return Colors[index % _countof(Colors)];
}
