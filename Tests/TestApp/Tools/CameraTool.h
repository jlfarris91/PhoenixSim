
#pragma once

#include <SDL3/SDL_rect.h>

#include "Optional.h"
#include "Reflection.h"
#include "../SDL/SDLCamera.h"
#include "../SDL/SDLTool.h"

namespace Phoenix
{
    struct SDLViewport;
    struct SDLCamera;
    class Session;

    struct CameraTool : ISDLTool
    {
        PHX_DECLARE_TYPE_BEGIN(CameraTool)
            PHX_REGISTER_FIELD(float, PanSpeed)
            PHX_REGISTER_FIELD(float, ZoomSpeed)
        PHX_DECLARE_TYPE_END()

        CameraTool(Session* session, SDLCamera* camera, SDLViewport* viewport);

        void OnAppRenderWorld(WorldConstRef& world, SDLDebugState& state, SDLDebugRenderer& renderer) override;
        void OnAppRenderUI(ImGuiIO& io) override;
        void OnAppEvent(SDLDebugState& state, SDL_Event* event) override;

        Session* Session;
        SDLCamera* Camera;
        SDLViewport* Viewport;
        TOptional<SDL_FPoint> CameraDragPos;
        float PanSpeed = 100.0f;
        float ZoomSpeed = 0.1f;
    };
}