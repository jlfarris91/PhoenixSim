
#pragma once

#include "FeatureNavMesh.h"
#include "FixedVector.h"
#include "Optional.h"
#include "Reflection.h"
#include "../SDL/SDLTool.h"

namespace Phoenix
{
    class Session;

    struct NavMeshTool : ISDLTool
    {
        DECLARE_TYPE_BEGIN(NavMeshTool)
            REGISTER_FIELD(float, BrushSize)
            REGISTER_FIELD(bool, bDrawVertCircles)
            REGISTER_FIELD(bool, bDrawOpenSet)
            REGISTER_FIELD(bool, bDrawVertIds)
            REGISTER_FIELD(bool, bDrawHalfEdgeIds)
            REGISTER_FIELD(bool, bDrawFaceIds)
            REGISTER_FIELD(bool, bDrawFaceCircumcircles)
            REGISTER_FIELD(bool, bDrawPathPortals)
            REGISTER_FIELD(float, AgentRadius)
        DECLARE_TYPE_END()

        NavMeshTool(Session* session);

        void OnAppRenderWorld(WorldConstRef& world, SDLDebugState& state, SDLDebugRenderer& renderer) override;
        void OnAppRenderUI(ImGuiIO& io) override;
        void OnAppEvent(SDLDebugState& state, SDL_Event* event) override;

        void RenderMesh(SDLDebugState& state, SDLDebugRenderer& renderer, const Pathfinding::NavMesh& mesh);

        void RenderPath(
            SDLDebugState& state,
            SDLDebugRenderer& renderer,
            const Pathfinding::NavMesh& mesh,
            const TMeshPath<Pathfinding::NavMesh>& meshPath);

        Session* Session;

        float BrushSize = 10.0f;
        bool bDrawVertCircles = true;
        bool bDrawOpenSet = true;
        bool bDrawVertIds = true;
        bool bDrawHalfEdgeIds = true;
        bool bDrawFaceIds = true;
        bool bDrawFaceCircumcircles = false;
        bool bDrawPathPortals = false;

        TOptional<Vec2> LineStart, LineEnd;

        TOptional<Vec2> PathStart, PathGoal;
        float AgentRadius = 10.0;
    };
}
