
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

bool SDLDebugState::MouseDown(uint8 button) const
{
    auto iter = MouseButtonStates.find(button);
    return iter != MouseButtonStates.end() && iter->second;
}

bool SDLDebugState::MouseUp(uint8 button) const
{
    auto iter = MouseButtonStates.find(button);
    return iter == MouseButtonStates.end() || !iter->second;
}

Vec2 SDLDebugState::GetWorldMousePos() const
{
    float mx, my;
    SDL_GetMouseState(&mx, &my);

    return Viewport->ViewportPosToWorldPos(SDL_FPoint(mx, my));
}

void SDLDebugState::ProcessAppEvent(void* appstate, SDL_Event* event)
{
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
