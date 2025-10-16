
#include "EntityTool.h"

#include <SDL3/SDL_events.h>

#include "Actions.h"
#include "FixedVector.h"
#include "Session.h"
#include "../SDL/SDLDebugState.h"
#include "../SDL/SDLDebugRenderer.h"

using namespace Phoenix;

EntityTool::EntityTool(Phoenix::Session* session)
    : Session(session)
{
}

void EntityTool::OnAppRenderWorld(WorldConstRef& world, SDLDebugState& state, SDLDebugRenderer& renderer)
{
    Vec2 mouseWorldPos = state.GetWorldMousePos();

    if (state.KeyDown(SDLK_X))
    {
        renderer.DrawCircle(mouseWorldPos, BrushSize, Color::White);
    }

    if (state.KeyDown(SDLK_F))
    {
        renderer.DrawCircle(mouseWorldPos, BrushSize, Color::White);
    }
}

void EntityTool::OnAppRenderUI(ImGuiIO& io)
{
    
}

void EntityTool::OnAppEvent(SDLDebugState& state, SDL_Event* event)
{
    Vec2 mouseWorldPos = state.GetWorldMousePos();

    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        // Spawn entities
        if (state.MouseButtonDown(SDL_BUTTON_LEFT))
        {
            Action action;
            action.Verb = "spawn_entity"_n;
            action.Data[0].Name = "Unit"_n;
            action.Data[1].Distance = mouseWorldPos.X;
            action.Data[2].Distance = mouseWorldPos.Y;
            action.Data[3].Degrees = Vec2::RandUnitVector().AsRadians();
            action.Data[4].UInt32 = SpawnCount;
            Session->QueueAction(action);
        }

        // Spawn moving entities
        if (state.KeyDown(SDLK_S))
        {
            Action action;
            action.Verb = "spawn_entity"_n;
            action.Data[0].Name = "Unit"_n;
            action.Data[1].Distance = mouseWorldPos.X;
            action.Data[2].Distance = mouseWorldPos.Y;
            action.Data[3].Degrees = Vec2::RandUnitVector().AsRadians();
            action.Data[4].UInt32 = SpawnCount;
            action.Data[5].Speed = MoveSpeed;
            Session->QueueAction(action);
        }

        // Release entities
        if (state.KeyDown(SDLK_X))
        {
            Action action;
            action.Verb = "release_entities_in_range"_n;
            action.Data[0].Distance = mouseWorldPos.X;
            action.Data[1].Distance = mouseWorldPos.Y;
            action.Data[2].Distance = BrushSize;
            Session->QueueAction(action);
        }
    }
}
