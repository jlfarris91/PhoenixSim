
#pragma once

#include <SDL3/SDL_video.h>

#include "FixedPoint/FixedVector.h"

namespace Phoenix
{
    struct SDLCamera;

    struct SDLViewport
    {
        SDLViewport(SDL_Window* window, SDLCamera* camera);

        Vec2 ViewportPosToWorldPos(const SDL_FPoint& pos) const;
        Vec2 ViewportVecToWorldVec(const SDL_FPoint& vec) const;

        SDL_FPoint WorldPosToViewportPos(const Vec2& pos) const;
        SDL_FPoint WorldVecToViewportVec(const Vec2& vec) const;

        SDL_Window* Window;
        SDLCamera* Camera;
    };
}
