
#pragma once

#include "FeatureNavMesh.h"
#include "FixedPoint/FixedVector.h"
#include "Optional.h"
#include "Reflection.h"
#include "../SDL/SDLTool.h"

namespace Phoenix
{
    class Session;

    struct NavMeshTool : ISDLTool
    {
        PHX_DECLARE_TYPE_BEGIN(NavMeshTool)
            PHX_REGISTER_FIELD(float, BrushSize)
            PHX_REGISTER_FIELD(bool, bDrawVertCircles)
            PHX_REGISTER_FIELD(bool, bDrawOpenSet)
            PHX_REGISTER_FIELD(bool, bDrawVertIds)
            PHX_REGISTER_FIELD(bool, bDrawHalfEdgeIds)
            PHX_REGISTER_FIELD(bool, bDrawFaceIds)
            PHX_REGISTER_FIELD(bool, bDrawFaceCircumcircles)
            PHX_REGISTER_FIELD(bool, bDrawPathPortals)
            PHX_REGISTER_FIELD(float, AgentRadius)
            PHX_REGISTER_FIELD(PHXString, MapDir)
            PHX_REGISTER_METHOD(LoadMeshFromFile)
        PHX_DECLARE_TYPE_END()

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

        void LoadMeshFromFile();

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

        using SGDistance = TFixed<14>;
        using SGVec2 = TVec2<SGDistance>;

        PHXString MapDir = "C:\\Pegasus\\pegasus-main-1\\PegasusGame\\Pegasus\\Content\\data\\maps\\TitansCausewayV2";
        std::vector<SGVec2> LoadedVerts;
        uint32 LoadedVertIndex = 0;
    };
}
