
#include "SDLDebugState.h"

#include <SDL3/SDL_mouse.h>

using namespace Phoenix;

SDLDebugState::SDLDebugState(SDLViewport* viewport)
    : Viewport(viewport)
{
}

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

bool SDLDebugState::KeyPressed(uint32 keycode) const
{
    if (!KeyDown(keycode))
        return false;

    auto prevIter = PrevKeyStates.find(keycode);
    return prevIter == PrevKeyStates.end() || !prevIter->second;
}

bool SDLDebugState::KeyReleased(uint32 keycode) const
{
    if (!KeyUp(keycode))
        return false;

    auto prevIter = PrevKeyStates.find(keycode);
    return prevIter != PrevKeyStates.end() && prevIter->second;
}

bool SDLDebugState::MouseButtonDown(uint8 button) const
{
    auto iter = MouseButtonStates.find(button);
    return iter != MouseButtonStates.end() && iter->second;
}

bool SDLDebugState::MouseButtonUp(uint8 button) const
{
    auto iter = MouseButtonStates.find(button);
    return iter == MouseButtonStates.end() || !iter->second;
}

bool SDLDebugState::MouseButtonPressed(uint8 button) const
{
    if (!MouseButtonDown(button))
        return false;

    auto prevIter = PrevMouseButtonStates.find(button);
    return prevIter == PrevMouseButtonStates.end() || !prevIter->second;
}

bool SDLDebugState::MouseButtonReleased(uint8 button) const
{
    if (!MouseButtonUp(button))
        return false;

    auto prevIter = PrevMouseButtonStates.find(button);
    return prevIter != PrevMouseButtonStates.end() && prevIter->second;
}

Vec2 SDLDebugState::GetWorldMousePos() const
{
    float mx, my;
    SDL_GetMouseState(&mx, &my);

    return Viewport->ViewportPosToWorldPos(SDL_FPoint(mx, my));
}

void SDLDebugState::ProcessAppEvent(SDL_Event* event)
{
    PrevKeyStates = KeyStates;
    PrevMouseButtonStates = MouseButtonStates;

    if (event->type == SDL_EVENT_KEY_DOWN)
    {
        KeyStates[event->key.key] = true;
    }

    if (event->type == SDL_EVENT_KEY_UP)
    {
        KeyStates[event->key.key] = false;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        MouseButtonStates[event->button.button] = true;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        MouseButtonStates[event->button.button] = false;
    }
}
