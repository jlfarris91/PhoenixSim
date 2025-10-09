
#pragma once

#include <SDL3/SDL_keycode.h>

#include "Debug.h"
#include "PhoenixCore.h"

namespace Phoenix
{
    struct SDLDebugState : IDebugState
    {
        bool KeyDown(uint32 keycode) const override;
        bool KeyUp(uint32 keycode) const override;

        bool MouseDown(uint32 keycode) const override;
        bool MouseUp(uint32 keycode) const override;

        Vec2 GetMousePos() const override;

        TMap<uint32, bool> MouseButtonStates;
        TMap<SDL_Keycode, bool> KeyStates;
        Vec2 MapCenter;
    };
}
