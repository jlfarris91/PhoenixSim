
#include "FeatureNavMesh.h"

#include "Color.h"
#include "Debug.h"
#include "FixedVector.h"
#include "Profiling.h"

using namespace Phoenix;
using namespace Phoenix::Pathfinding;

FeatureNavMesh::FeatureNavMesh()
{
}

void FeatureNavMesh::OnPreUpdate(WorldRef world, const FeatureUpdateArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    IFeature::OnPreUpdate(world, args);

    FeatureNavMeshDynamicBlock& block = world.GetBlockRef<FeatureNavMeshDynamicBlock>();

    auto bl = Vec2(0, 0);
    auto br = Vec2(192, 0);
    auto tl = Vec2(0, 192);
    auto tr = Vec2(192, 192);

    block.DynamicNavMesh.Reset();
    block.DynamicNavMesh.InsertFace(bl, tr, tl, 1);
    block.DynamicNavMesh.InsertFace(bl, br, tr, 2);

    for (const auto& point : block.DynamicPoints)
    {
        block.DynamicNavMesh.CDT_InsertPoint(point);
    }

    for (const auto& edge : block.DynamicEdges)
    {
        block.DynamicNavMesh.CDT_InsertEdge(edge);
    }
}

void FeatureNavMesh::OnHandleAction(WorldRef world, const FeatureActionArgs& action)
{
    IFeature::OnHandleAction(world, action);

    FeatureNavMeshDynamicBlock& dynamicBlock = world.GetBlockRef<FeatureNavMeshDynamicBlock>();
    FeatureNavMeshScratchBlock& scratchBlock = world.GetBlockRef<FeatureNavMeshScratchBlock>();

    if (action.Action.Verb == "insert_point"_n)
    {
        auto ptx = action.Action.Data[0].Distance;
        auto pty = action.Action.Data[1].Distance;
        dynamicBlock.DynamicPoints.EmplaceBack(ptx, pty);
    }

    if (action.Action.Verb == "insert_edge"_n)
    {
        auto pt0x = action.Action.Data[0].Distance;
        auto pt0y = action.Action.Data[1].Distance;
        auto pt1x = action.Action.Data[2].Distance;
        auto pt1y = action.Action.Data[3].Distance;
        dynamicBlock.DynamicEdges.EmplaceBack(Vec2{ pt0x, pt0y }, Vec2{ pt1x, pt1y });
    }

    if (action.Action.Verb == "find_path"_n)
    {
        auto pt0x = action.Action.Data[0].Distance;
        auto pt0y = action.Action.Data[1].Distance;
        auto pt1x = action.Action.Data[2].Distance;
        auto pt1y = action.Action.Data[3].Distance;
        auto r = action.Action.Data[4].Distance;
        scratchBlock.MeshPath.FindPath(dynamicBlock.DynamicNavMesh, { pt0x, pt0y }, { pt1x, pt1y }, r, false);
        if (scratchBlock.MeshPath.LastStepResult == TMeshPath<>::EStepResult::FoundPath)
        {
            scratchBlock.MeshPath.ResolvePath(dynamicBlock.DynamicNavMesh, false);
        }
    }

    if (action.Action.Verb == "delete_edges_and_points"_n)
    {
        Vec2 pos = {action.Action.Data[0].Distance, action.Action.Data[1].Distance};
        Distance radius = action.Action.Data[2].Distance;

        for (size_t i = 0; i < dynamicBlock.DynamicPoints.Num();)
        {
            if (Vec2::Distance(dynamicBlock.DynamicPoints[i], pos) < radius)
            {
                dynamicBlock.DynamicPoints.RemoveAt(i);
            }
            else
            {
                ++i;
            }
        }

        for (size_t i = 0; i < dynamicBlock.DynamicEdges.Num();)
        {
            if (Line2::DistanceToLine(dynamicBlock.DynamicEdges[i], pos) < radius)
            {
                dynamicBlock.DynamicEdges.RemoveAt(i);
            }
            else
            {
                ++i;
            }
        }
    }
}

void FeatureNavMesh::OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer)
{
    IFeature::OnDebugRender(world, state, renderer);

    const FeatureNavMeshDynamicBlock& block = world.GetBlockRef<FeatureNavMeshDynamicBlock>();

    const NavMesh& mesh = block.DynamicNavMesh;
    Vec2 cursorPos = state.GetWorldMousePos();

    if (bDebugDrawVertices)
    {
        for (const auto& vert : mesh.Vertices)
        {
            renderer.DrawCircle(vert, 3.0f, Color::White);
        }
    }

    if (bDebugDrawHalfEdges)
    {
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
            auto result = mesh.IsPointInFace(i, cursorPos);
            if (result.Result == EPointInFaceResult::Inside)
            {
                Color color = renderer.GetColor(i) / 2;

                mesh.ForEachHalfEdgeInFace(i, [&](const auto& halfEdge)
                {
                    const Vec2& vertA = mesh.Vertices[halfEdge.VertA];
                    const Vec2& vertB = mesh.Vertices[halfEdge.VertB];
                    renderer.DrawLine(vertA, vertB, color);
                });
            }
        }
    }

    if (bDebugDrawVertexIds)
    {
        for (size_t i = 0; i < mesh.Vertices.Num(); ++i)
        {
            const Vec2& pt = mesh.Vertices[i];

            char str[256] = { '\0' };
            sprintf_s(str, _countof(str), "%llu", i);
            renderer.DrawDebugText(pt, str, _countof(str), Color::White);
        }
    }

    if (bDebugDrawHalfEdgeIds)
    {
        for (size_t i = 0; i < mesh.HalfEdges.Num(); ++i)
        {
            if (!mesh.IsValidHalfEdge(i))
                continue;

            Vec2 center, normal;
            mesh.GetEdgeCenterAndNormal(i, center, normal);
            Vec2 pt = center + normal * 10.0;

            char str[256] = { '\0' };
            sprintf_s(str, _countof(str), "%llu", i);
            renderer.DrawDebugText(pt, str, _countof(str), Color::White);
        }
    }

    if (bDebugDrawFaceIds)
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
            sprintf_s(str, _countof(str), "%llu", i);
            renderer.DrawDebugText(center, str, _countof(str), color);
        }
    }
}

NavMesh::TIndex FeatureNavMesh::InsertPoint(WorldRef world, const NavMesh::TVert& pt)
{
    FeatureNavMeshDynamicBlock& block = world.GetBlockRef<FeatureNavMeshDynamicBlock>();
    return block.DynamicNavMesh.CDT_InsertPoint(pt);
}

bool FeatureNavMesh::InsertEdge(WorldRef world, const NavMesh::TVert& start, const NavMesh::TVert& end)
{
    FeatureNavMeshDynamicBlock& block = world.GetBlockRef<FeatureNavMeshDynamicBlock>();
    return block.DynamicNavMesh.CDT_InsertEdge({ start, end });
}

PathResult FeatureNavMesh::PathTo(
    WorldConstRef world,
    const NavMesh::TVert& start,
    const NavMesh::TVert& goal,
    NavMesh::TVertComp radius)
{
    const FeatureNavMeshDynamicBlock& dynamicBlock = world.GetBlockRef<FeatureNavMeshDynamicBlock>();
    const FeatureNavMeshScratchBlock& scratchBlock = world.GetBlockRef<FeatureNavMeshScratchBlock>();

    scratchBlock.MeshPath.FindPath(dynamicBlock.DynamicNavMesh, start, goal, radius, false);

    return { scratchBlock.MeshPath.LastStepResult == TMeshPath<>::EStepResult::FoundPath, scratchBlock.MeshPath.Path[1] };
}

PathResult FeatureNavMesh::CanPathTo(
    WorldConstRef world,
    const NavMesh::TVert& start,
    const NavMesh::TVert& goal,
    NavMesh::TVertComp radius)
{
    const FeatureNavMeshDynamicBlock& dynamicBlock = world.GetBlockRef<FeatureNavMeshDynamicBlock>();
    const FeatureNavMeshScratchBlock& scratchBlock = world.GetBlockRef<FeatureNavMeshScratchBlock>();

    scratchBlock.MeshPath.FindPath(dynamicBlock.DynamicNavMesh, start, goal, radius, false);

    return { scratchBlock.MeshPath.LastStepResult == TMeshPath<>::EStepResult::FoundPath, scratchBlock.MeshPath.Path[1] };
}