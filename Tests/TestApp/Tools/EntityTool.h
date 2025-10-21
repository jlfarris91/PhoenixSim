
#pragma once

#include "Reflection.h"
#include "../SDL/SDLTool.h"

namespace Phoenix
{
    class Session;

    struct EntityTool : ISDLTool
    {
        PHX_DECLARE_TYPE_BEGIN(EntityTool)
            PHX_REGISTER_FIELD(float, BrushSize)
            PHX_REGISTER_FIELD(uint32, SpawnCount)
            PHX_REGISTER_FIELD(float, MoveSpeed)
            PHX_REGISTER_FIELD(float, PushForce)
        PHX_DECLARE_TYPE_END()

        EntityTool(Session* session);

        void OnAppRenderWorld(WorldConstRef& world, SDLDebugState& state, SDLDebugRenderer& renderer) override;
        void OnAppRenderUI(ImGuiIO& io) override;
        void OnAppEvent(SDLDebugState& state, SDL_Event* event) override;

        Session* Session;
        float BrushSize = 10.0f;
        uint32 SpawnCount = 1;
        float MoveSpeed = 10.0f;
        float PushForce = 100.0f;
    };
    
}
