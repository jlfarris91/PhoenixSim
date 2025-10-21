
#include "NavMeshTool.h"

#include <fstream>
#include <SDL3/SDL_events.h>

#include "Actions.h"
#include "FeatureNavMesh.h"
#include "FixedVector.h"
#include "Session.h"
#include "../SDL/SDLDebugState.h"
#include "../SDL/SDLDebugRenderer.h"
#include "nlohmann/json.hpp"

using namespace Phoenix;
using namespace Phoenix::Pathfinding;

NavMeshTool::NavMeshTool(Phoenix::Session* session)
    : Session(session)
{
}

void NavMeshTool::OnAppRenderWorld(WorldConstRef& world, SDLDebugState& state, SDLDebugRenderer& renderer)
{
    Vec2 mouseWorldPos = state.GetWorldMousePos();

    if (const FeatureNavMeshDynamicBlock* dynamicBlock = world.GetBlock<FeatureNavMeshDynamicBlock>())
    {
        RenderMesh(state, renderer, dynamicBlock->DynamicNavMesh);

        for (const Vec2& vert : dynamicBlock->DynamicPoints)
        {
            renderer.DrawCircle(vert, 0.1f, Color::Red);
        }

        if (const FeatureNavMeshScratchBlock* scratchBlock = world.GetBlock<FeatureNavMeshScratchBlock>())
        {
            RenderPath(state, renderer, dynamicBlock->DynamicNavMesh, scratchBlock->MeshPath);
        }
    }

    if (state.KeyDown(SDLK_X))
    {
        renderer.DrawCircle(mouseWorldPos, BrushSize, Color::White);
    }
}

void NavMeshTool::OnAppRenderUI(ImGuiIO& io)
{
}

void NavMeshTool::OnAppEvent(SDLDebugState& state, SDL_Event* event)
{
    Vec2 mouseWorldPos = state.GetWorldMousePos();

    if (state.KeyDown(SDLK_E))
    {
        LineEnd = mouseWorldPos;
    }

    if (state.KeyDown(SDLK_X))
    {
        Action action;
        action.Verb = "delete_edges_and_points"_n;
        action.Data[0].Distance = mouseWorldPos.X;
        action.Data[1].Distance = mouseWorldPos.Y;
        action.Data[2].Distance = BrushSize;
        Session->QueueAction(action);
    }

    if (state.KeyPressed(SDLK_E))
    {
        LineStart = mouseWorldPos;
        LineEnd.Reset();
    }

    if (state.KeyReleased(SDLK_E))
    {
        if (LineStart.IsSet() && LineEnd.IsSet() && (*LineStart - *LineEnd).Length() > 10.0f)
        {
            Action action;
            action.Verb = "insert_edge"_n;
            action.Data[0].Distance = LineStart->X;
            action.Data[1].Distance = LineStart->Y;
            action.Data[2].Distance = LineEnd->X;
            action.Data[3].Distance = LineEnd->Y;
            Session->QueueAction(action);
        }
        else if (LineStart.IsSet())
        {
            Action action;
            action.Verb = "insert_point"_n;
            action.Data[0].Distance = LineStart->X;
            action.Data[1].Distance = LineStart->Y;
            Session->QueueAction(action);
        }

        LineStart.Reset();
        LineEnd.Reset();
    }

    if (state.KeyPressed(SDLK_P))
    {
        if (PathStart.IsSet() && PathGoal.IsSet())
        {
            PathStart.Reset();
            PathGoal.Reset();
        }

        if (!PathStart.IsSet())
        {
            PathStart = mouseWorldPos;
        }
        else if (!PathGoal.IsSet())
        {
            PathGoal = mouseWorldPos;

            Action action;
            action.Verb = "find_path"_n;
            action.Data[0].Distance = PathStart->X;
            action.Data[1].Distance = PathStart->Y;
            action.Data[2].Distance = PathGoal->Y;
            action.Data[3].Distance = PathGoal->Y;
            action.Data[4].Distance = AgentRadius;
            Session->QueueAction(action);
        }
    }

    if (state.KeyDown(SDLK_SPACE) && LoadedVertIndex < LoadedVerts.size())
    {
        Action action;
        action.Verb = "insert_point"_n;
        action.Data[0].Distance = LoadedVerts[LoadedVertIndex].X;
        action.Data[1].Distance = LoadedVerts[LoadedVertIndex].Y;
        Session->QueueAction(action);
        LoadedVertIndex++;
    }
}

void NavMeshTool::RenderMesh(SDLDebugState& state, SDLDebugRenderer& renderer, const NavMesh& mesh)
{
    Vec2 mouseWorldPos = state.GetWorldMousePos();

    if (bDrawVertCircles)
    {
        for (const Vec2& vert : mesh.Vertices)
        {
            renderer.DrawCircle(vert, 0.1f, Color::White);
        }
    }

    // Draw unlocked edges gray
    for (const auto& edge : mesh.HalfEdges)
    {
        if (!mesh.Faces.IsValidIndex(edge.Face))
            continue;

        if (edge.bLocked)
            continue;

        const Vec2& vertA = mesh.Vertices[edge.VertA];
        const Vec2& vertB = mesh.Vertices[edge.VertB];
        renderer.DrawLine(vertA, vertB, Color(50, 50, 50));
    }

    // Draw locked edges in red
    for (const auto& edge : mesh.HalfEdges)
    {
        if (!mesh.Faces.IsValidIndex(edge.Face))
            continue;

        if (!edge.bLocked)
            continue;

        const Vec2& vertA = mesh.Vertices[edge.VertA];
        const Vec2& vertB = mesh.Vertices[edge.VertB];
        renderer.DrawLine(vertA, vertB, Color::Red);
    }

    // Redraw the edges of the face the mouse is within so that they draw on top
    for (size_t i = 0; i < mesh.Faces.Num(); ++i)
    {
        auto result = mesh.IsPointInFace(int16(i), mouseWorldPos);
        if (result.Result == EPointInFaceResult::Inside)
        {
            Color color = renderer.GetColor(i);

            mesh.ForEachHalfEdgeInFace(uint16(i), [&](const auto& halfEdge)
            {
                const Vec2& vertA = mesh.Vertices[halfEdge.VertA];
                const Vec2& vertB = mesh.Vertices[halfEdge.VertB];
                renderer.DrawLine(vertA, vertB, color);
            });
        }
    }

    if (bDrawVertIds)
    {
        for (size_t i = 0; i < mesh.Vertices.Num(); ++i)
        {
            const Vec2& pt = mesh.Vertices[i];

            char str[256];
            size_t len = sprintf_s(str, _countof(str), "%llu", i);
            renderer.DrawDebugText(pt, str, len, Color::White);
        }
    }

    if (bDrawHalfEdgeIds)
    {
        for (size_t i = 0; i < mesh.HalfEdges.Num(); ++i)
        {
            if (!mesh.IsValidHalfEdge(i))
                continue;

            Vec2 center, normal;
            mesh.GetEdgeCenterAndNormal(i, center, normal);
            Vec2 pt = center + normal * 10.0;

            char str[256] = { '\0' };
            size_t len = sprintf_s(str, _countof(str), "%llu", i);
            renderer.DrawDebugText(pt, str, len, Color::White);
        }
    }

    if (bDrawFaceIds)
    {
        for (size_t i = 0; i < mesh.Faces.Num(); ++i)
        {
            if (!mesh.Faces.IsValidIndex(i))
                continue;

            const auto& face = mesh.Faces[i];
            if (!mesh.HalfEdges.IsValidIndex(face.HalfEdge))
                continue;

            Color color = renderer.GetColor(i);

            Vec2 center;
            mesh.GetFaceCenter(i, center);

            char str[256] = { '\0' };
            size_t len = sprintf_s(str, _countof(str), "%llu", i);
            renderer.DrawDebugText(center, str, len, color);
        }
    }

    if (bDrawFaceCircumcircles)
    {
        for (int32 i = 0; i < mesh.Faces.Num(); ++i)
        {
            if (!mesh.Faces.IsValidIndex(i))
                continue;

            const auto& face = mesh.Faces[i];
            if (!mesh.HalfEdges.IsValidIndex(face.HalfEdge))
                continue;

            Color color = renderer.GetColor(i) / 2;

            const auto& e0 = mesh.HalfEdges[face.HalfEdge];
            const auto& e1 = mesh.HalfEdges[e0.Next];
            const auto& e2 = mesh.HalfEdges[e1.Next];
    
            const Vec2& a = mesh.Vertices[e0.VertA];
            const Vec2& b = mesh.Vertices[e1.VertA];
            const Vec2& c = mesh.Vertices[e2.VertA];
    
            auto aax = a.X * a.X;
            auto aay = a.Y * a.Y;
            auto a2 = a.X*a.X + a.Y*a.Y;
            auto b2 = b.X*b.X + b.Y*b.Y;
            auto c2 = c.X*c.X + c.Y*c.Y;
    
            auto d = 2 * (a.X * (b.Y - c.Y) + b.X * (c.Y - a.Y) + c.X * (a.Y - b.Y));
            if (d == 0)
                continue;
    
            auto ux = (a2 * (b.Y - c.Y) + b2 * (c.Y - a.Y) + c2 * (a.Y - b.Y)) / d;
            auto uy = (a2 * (c.X - b.X) + b2 * (a.X - c.X) + c2 * (b.X - a.X)) / d;
            auto r1 = (ux - a.X)*(ux - a.X) + (uy - a.Y)*(uy - a.Y);

            if (r1 < 0)
                continue;

            auto r = Sqrt(r1);

            renderer.DrawCircle({ ux, uy }, r, color);
        }
    }

    if (LineStart.IsSet() && LineEnd.IsSet() && !Vec2::Equals(*LineStart,*LineEnd))
    {
        renderer.DrawLine(*LineStart, *LineEnd, Color::White);

        for (auto edge : mesh.HalfEdges)
        {
            if (edge.Face == Index<uint16>::None)
            {
                continue;
            }

            const Vec2& a = mesh.Vertices[edge.VertA];
            const Vec2& b = mesh.Vertices[edge.VertB];

            Vec2 pt;
            if (Vec2::Intersects(a, b, *LineStart, *LineEnd, pt))
            {
                const Vec2& vertA = mesh.Vertices[edge.VertA];
                const Vec2& vertB = mesh.Vertices[edge.VertB];
                renderer.DrawLine(vertA, vertB, Color::White);

                renderer.DrawCircle(pt, 10, Color::White);
            }
        }
    }
}

void NavMeshTool::RenderPath(
    SDLDebugState& state,
    SDLDebugRenderer& renderer,
    const NavMesh& mesh,
    const TMeshPath<NavMesh>& meshPath)
{
    if (PathStart.IsSet())
    {
        renderer.DrawCircle(*PathStart, AgentRadius, Color::Blue);
    }

    if (PathGoal.IsSet())
    {
        renderer.DrawCircle(*PathGoal, AgentRadius, Color::Blue);
    }

    if (bDrawOpenSet)
    {
        for (uint16 halfEdgeIndex : meshPath.OpenSet)
        {
            const auto& halfEdge = mesh.HalfEdges[halfEdgeIndex];
        
            const Vec2& vertA = mesh.Vertices[halfEdge.VertA];
            const Vec2& vertB = mesh.Vertices[halfEdge.VertB];
            renderer.DrawLine(vertA, vertB, Color::Green);
        
            Vec2 center, normal;
            mesh.GetEdgeCenterAndNormal(halfEdgeIndex, center, normal);
            Vec2 f = center + normal * 10.0f;
        
            renderer.DrawLine(center, f, Color::Green);
        }
    }

    if (bDrawPathPortals)
    {
        for (int32 i = 0; i < (int32)meshPath.PathEdges.Num(); ++i)
        {
            const auto& halfEdge = mesh.HalfEdges[meshPath.PathEdges[i]];

            const Vec2& vertA = mesh.Vertices[halfEdge.VertA];
            const Vec2& vertB = mesh.Vertices[halfEdge.VertB];
            renderer.DrawLine(vertA, vertB, Color(100, 0, 100));
        }

        for (const Line2& line : meshPath.Funnel.PathDebugLines)
        {
            renderer.DrawLine(line.Start, line.End, Color::White);
        }

        renderer.DrawCircle(meshPath.Funnel.PortalApex, 5.0, Color::Green);
        renderer.DrawLine(meshPath.Funnel.PortalApex, meshPath.Funnel.PortalLeft, Color::Green);
        renderer.DrawLine(meshPath.Funnel.PortalApex, meshPath.Funnel.PortalRight, Color::Blue);
    }

    {
        for (int32 i = 0; i < (int32)meshPath.Path.Num() - 1; ++i)
        {
            const Vec2& v0 = meshPath.Path[i + 0];
            const Vec2& v1 = meshPath.Path[i + 1];
            renderer.DrawLine(v0, v1, Color::Blue);
        }
    }
}

void NavMeshTool::LoadMeshFromFile()
{
    PHXString mapFilePath = MapDir + "\\map.json";
    PHXString pathingMeshFilePath = MapDir + "\\pathing_mesh.json";

    std::ifstream mapFile(mapFilePath);
    std::ifstream pathingMeshFile(pathingMeshFilePath);
    if (!mapFile.is_open() || !pathingMeshFile.is_open())
    {
        return;
    }

    auto mapJson = nlohmann::json::parse(mapFile);

    SGVec2 mapSize;
    mapSize.X = mapJson.at("/dimensions/0"_json_pointer).get<int32>();
    mapSize.Y = mapJson.at("/dimensions/1"_json_pointer).get<int32>();
    
    auto pathingMeshJson = nlohmann::json::parse(pathingMeshFile);

    LoadedVerts.clear();
    LoadedVertIndex = 0;

    for (auto & vert : pathingMeshJson["verts"])
    {
        SGDistance x = mapSize.X / 2 + SGDistance(Q32(vert[0].get<int32>()));
        SGDistance y = mapSize.Y / 2 + SGDistance(Q32(vert[1].get<int32>()));
        LoadedVerts.emplace_back(x, y);
    }

    {
        Action action;
        action.Verb = "set_nav_mesh_size"_n;
        action.Data[0].Distance = mapSize.X;
        action.Data[1].Distance = mapSize.Y;
        Session->QueueAction(action);
    }

    for (const SGVec2& vert : LoadedVerts)
    {
        Action action;
        action.Verb = "insert_point"_n;
        action.Data[0].Distance = vert.X;
        action.Data[1].Distance = vert.Y;
        Session->QueueAction(action);
    }

    for (auto & vert : pathingMeshJson["edges"])
    {
        uint32 vertAIdx = vert[0].get<uint32>();
        uint32 vertBIdx = vert[1].get<uint32>();
    
        const SGVec2& vertA = LoadedVerts[vertAIdx];
        const SGVec2& vertB = LoadedVerts[vertBIdx];
    
        Action action;
        action.Verb = "insert_edge"_n;
        action.Data[0].Distance = vertA.X;
        action.Data[1].Distance = vertA.Y;
        action.Data[2].Distance = vertB.X;
        action.Data[3].Distance = vertB.Y;
        Session->QueueAction(action);
    }
}
