
#include "SDLDebugState.h"

#include <SDL3/SDL_mouse.h>

using namespace Phoenix;

bool SDLDebugState::KeyDown(uint32 keycode) const
{
    auto iter = KeyStates.find(keycode);
    return iter != KeyStates.end() && iter->second;
}

bool SDLDebugState::KeyUp(uint32 keycode) const
{
    auto iter = KeyStates.find(keycode);
    return iter == KeyStates.end() || !iter->second;
}

bool SDLDebugState::MouseDown(uint32 keycode) const
{
    auto iter = MouseButtonStates.find(keycode);
    return iter != MouseButtonStates.end() && iter->second;
}

bool SDLDebugState::MouseUp(uint32 keycode) const
{
    auto iter = MouseButtonStates.find(keycode);
    return iter == MouseButtonStates.end() || !iter->second;
}

Vec2 SDLDebugState::GetMousePos() const
{
    float mx, my;
    SDL_GetMouseState(&mx, &my);

    return Vec2(mx - MapCenter.X, -(my - MapCenter.Y));
}
