
#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#include "Debug.h"
#include "PhoenixCore.h"
#include "SDLViewport.h"

namespace Phoenix
{
    struct SDLDebugState : IDebugState
    {
        SDLDebugState(SDLViewport* viewport);

        bool KeyDown(uint32 keycode) const override;
        bool KeyUp(uint32 keycode) const override;

        bool MouseButtonDown(uint8 button) const override;
        bool MouseButtonUp(uint8 button) const override;

        Vec2 GetWorldMousePos() const override;

        void ProcessAppEvent(void *appstate, SDL_Event* event);

        SDLViewport* Viewport;
        TMap<uint8, bool> MouseButtonStates;
        TMap<SDL_Keycode, bool> KeyStates;
    };
}
