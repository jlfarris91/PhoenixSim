
#pragma once

#include <algorithm>

#include "Mesh2.h"
#include "Containers/FixedQueue.h"
#include "Containers/FixedMap.h"

namespace Phoenix
{
    template <class TMesh = DefaultFixedCDTMesh2>
    struct TMeshFunnel;

    template <class TMesh = DefaultFixedCDTMesh2>
    struct TMeshPath
    {
        using TVert = typename TMesh::TVert;
        using TVertComp = typename TMesh::TVertComp;
        using TIdx = typename TMesh::TIndex;
        using THalfEdge = typename TMesh::THalfEdge;
        using TFace = typename TMesh::TFace;

        struct Node
        {
            Value G;
            Value H;
            TIdx FromEdge = Index<TIdx>::None;
            TVert Center;
            bool Discarded = false;
            bool Visited = false;
            Value GetF() const { return G + H; }
        };

        enum class EStepResult
        {
            Continue,
            FoundPath,
            Failed
        };

        TVert StartPos;
        TVert GoalPos;
        TVertComp Radius;
        TIdx StartFaceIndex = Index<TIdx>::None;
        TIdx GoalFaceIndex = Index<TIdx>::None;
        TIdx CurrEdgeIndex = Index<TIdx>::None;
        uint32 Steps = 0;
        TFixedQueue<TIdx, 1024> OpenSet;
        TFixedMap<TIdx, Node, 1024> Nodes;
        EStepResult LastStepResult = EStepResult::Continue;

        TFixedArray<Vec2, 1024> Path;
        TFixedArray<TIdx, 1024> PathEdges;

        TMeshFunnel<TMesh> Funnel;

        bool FindPath(
            const TMesh& mesh,
            const TVert& startPos,
            const TVert& goalPos,
            TVertComp radius,
            bool stepping)
        {
            StartPos = startPos;
            GoalPos = goalPos;
            Radius = radius;
            StartFaceIndex = mesh.FindFaceContainingPoint(startPos);
            GoalFaceIndex = mesh.FindFaceContainingPoint(goalPos);
            Steps = 0;
            OpenSet.Reset();
            Nodes.Reset();
            CurrEdgeIndex = Index<TIdx>::None;
            LastStepResult = EStepResult::Continue;

            if (StartFaceIndex == Index<TIdx>::None || GoalFaceIndex == Index<TIdx>::None)
            {
                return false;
            }

            // Start by adding all edges of the start face to the open set
            mesh.ForEachHalfEdgeIndexInFace(StartFaceIndex, [&, this](TIdx halfEdgeIndex)
            {
                // Ignore edges that can't be traversed
                if (mesh.IsEdgeLocked(halfEdgeIndex))
                    return;

                Node& startNode = FindOrAddNode(mesh, halfEdgeIndex);
                startNode.G = CalculateHeuristicToStart(halfEdgeIndex);

                if (!startNode.Discarded)
                {
                    OpenSet.Enqueue(halfEdgeIndex);
                }
            });

            if (!stepping)
            {
                while (!OpenSet.IsEmpty())
                {
                    EStepResult result = Step(mesh);
                    if (result != EStepResult::Continue)
                    {
                        break;
                    }
                }
            }

            return true;
        }

        EStepResult Step(const TMesh& mesh)
        {
            if (OpenSet.IsEmpty())
            {
                LastStepResult = EStepResult::Failed;
                return LastStepResult;
            }

            ++Steps;

            TIdx currEdgeIndex = OpenSet.Dequeue();
            CurrEdgeIndex = currEdgeIndex;

            if (!mesh.IsValidHalfEdge(currEdgeIndex))
            {
                LastStepResult = EStepResult::Continue;
                return LastStepResult;
            }

            Node& currNode = FindOrAddNode(mesh, currEdgeIndex);
            currNode.Visited = true;

            const THalfEdge& edge = mesh.HalfEdges[currEdgeIndex];
            if (edge.bLocked)
            {
                LastStepResult = EStepResult::Continue;
                return LastStepResult;
            }

            // Found the goal triangle, we're done.
            if (edge.Face == GoalFaceIndex)
            {
                // TODO reconstruct triangle path
                LastStepResult = EStepResult::FoundPath;
                return LastStepResult;
            }

            TIdx twinEdgeIndex = edge.Twin;
            if (!mesh.IsValidHalfEdge(twinEdgeIndex))
            {
                LastStepResult = EStepResult::Continue;
                return LastStepResult;
            }

            {
                const THalfEdge& twinHalfEdge0 = mesh.HalfEdges[twinEdgeIndex];

                if (twinHalfEdge0.Face == GoalFaceIndex)
                {
                    LastStepResult = EStepResult::FoundPath;
                    return LastStepResult;
                }

                // Skip the twin and check the next 2
                twinEdgeIndex = twinHalfEdge0.Next;

                for (size_t i = 0; i < 2; ++i)
                {
                    const THalfEdge& twinHalfEdgeN = mesh.HalfEdges[twinEdgeIndex];

                    if (twinHalfEdgeN.bLocked || !mesh.IsValidHalfEdge(twinHalfEdgeN.Twin))
                    {
                        twinEdgeIndex = twinHalfEdgeN.Next;
                        continue;
                    }

                    const THalfEdge& twinTwin = mesh.HalfEdges[twinHalfEdgeN.Twin];
                    if (Nodes.Contains(twinHalfEdgeN.Twin) && Nodes[twinHalfEdgeN.Twin].Visited)
                    {
                        twinEdgeIndex = twinHalfEdgeN.Next;
                        continue;
                    }

                    if (twinTwin.bLocked)
                    {
                        twinEdgeIndex = twinHalfEdgeN.Next;
                        continue;
                    }

                    {
                        PHX_ASSERT(currEdgeIndex != twinEdgeIndex);
                        Node& neighborNode = FindOrAddNode(mesh, twinEdgeIndex);

                        if (neighborNode.Discarded)
                            continue;

                        auto d = CalculateHeuristic(currEdgeIndex, twinEdgeIndex);
                        auto score = currNode.G + d;
                        if (score < neighborNode.G)
                        {
                            neighborNode.FromEdge = currEdgeIndex;
                            neighborNode.G = score;
                        }

                        if (!neighborNode.Visited)
                            OpenSet.Enqueue(twinEdgeIndex);
                    }

                    twinEdgeIndex = twinHalfEdgeN.Next;
                }
            }

            if (!OpenSet.IsEmpty())
            {
                std::sort(OpenSet.begin(), OpenSet.end(), [&](TIdx a, TIdx b)
                    {
                        return Nodes[a].GetF() < Nodes[b].GetF();
                    });
            }

            LastStepResult = EStepResult::Continue;
            return LastStepResult;
        }

        Node& FindOrAddNode(const TMesh& mesh, TIdx halfEdgeIndex)
        {
            if (!Nodes.Contains(halfEdgeIndex))
            {
                Node& node = Nodes.InsertDefaulted_GetRef(halfEdgeIndex);
                node.Visited = false;
                node.Discarded = mesh.GetEdgeLength(halfEdgeIndex) < Radius;
                mesh.GetEdgeCenter(halfEdgeIndex, node.Center);
                node.FromEdge = Index<TIdx>::None;
                node.G = Value::Max;
                node.H = CalculateHeuristicToGoal(halfEdgeIndex);
            }
            return Nodes[halfEdgeIndex];
        }

        Value CalculateHeuristicToGoal(TIdx edgeIndex) const
        {
            return Vec2::Distance(GoalPos, Nodes[edgeIndex].Center);
        }

        Value CalculateHeuristicToStart(TIdx halfEdgeIndex) const
        {
            return Vec2::Distance(StartPos, Nodes[halfEdgeIndex].Center);
        }

        Value CalculateHeuristic(TIdx currEdgeIndex, TIdx neighborEdgeIndex) const
        {
            return (Nodes[currEdgeIndex].Center - Nodes[neighborEdgeIndex].Center).Length();
        }

        void ResolvePath(const TMesh& mesh, bool stepping = false)
        {
            Path.Reset();
            Path.PushBack(GoalPos);

            PathEdges.Reset();

            // Populate the corridor
            {
                TIdx idx = CurrEdgeIndex;
                while (Nodes.Contains(idx))
                {
                    Node& node = Nodes[idx];
                    PathEdges.PushBack(idx);
                    idx = node.FromEdge;
                }
            }

            if (!stepping)
            {
                Funnel.Initialize(mesh, *this, Radius);
                while (!Funnel.Step(mesh, *this)) {}
            }
        }
    };

    template <class TMesh>
    struct TMeshFunnel
    {
        Vec2 StartPos;
        Vec2 GoalPos;
        Vec2 PortalApex;
        Vec2 PortalLeft;
        Vec2 PortalRight;
        int32 ChainIndex = 0;
        int32 ApexIndex = 0;
        int32 LeftIndex = 0;
        int32 RightIndex = 0;
        int32 PortalSide = 0;
        Distance Radius;
        TFixedArray<Vec2, 128> PathChainRhs;
        TFixedArray<Vec2, 128> PathChainLhs;
        TFixedArray<Line2, 128> PathDebugLines;

        void Initialize(const TMesh& mesh, const TMeshPath<TMesh>& path, Distance radius)
        {
            StartPos = path.StartPos;
            GoalPos = path.GoalPos;
            Radius = radius;

            PathChainRhs.Reset();
            PathChainLhs.Reset();
            PathDebugLines.Reset();

            PathChainRhs.PushBack(path.GoalPos);
            PathChainLhs.PushBack(path.GoalPos);
            mesh.TracePortalEdgeVerts(path.PathEdges, PathChainRhs, PathChainLhs);
            PathChainRhs.PushBack(path.StartPos);
            PathChainLhs.PushBack(path.StartPos);

            // Reverse the lhs chain so that it's CCW
            // std::reverse(chainLhs.begin(), chainLhs.end());

            // Start the anchor with the first
            PortalApex = path.GoalPos;

            ChainIndex = 0;
            ApexIndex = 0;
            LeftIndex = 0;
            RightIndex = 0;
            PortalSide = 0;
            PortalRight = PathChainRhs[0];
            PortalLeft = PathChainLhs[0];
        }

        bool Step(const TMesh& mesh, TMeshPath<TMesh>& path)
        {
            ++ChainIndex;

            if (!PathChainLhs.IsValidIndex(ChainIndex) || !PathChainRhs.IsValidIndex(ChainIndex))
            {
                return true;
            }

            const Vec2& right = PathChainRhs[ChainIndex];
            const Vec2& left = PathChainLhs[ChainIndex];

            if (TriArea2(PortalApex, PortalRight, right) <= 0.0)
            {
                if (Vec2::Equals(PortalApex, PortalRight) || TriArea2(PortalApex, PortalLeft, right) > 0)
                {
                    // Tighten the funnel
                    PortalRight = right;
                    RightIndex = ChainIndex;
                }
                else
                {
                    Vec2 v = PortalApex - PortalLeft;
                    Vec2 n = Vec2(-v.Y, v.X).Normalized();

                    auto sign = PortalSide == 1 ? 1 : -1;
                    Vec2 a = PortalApex + n * Radius * sign;
                    path.Path.PushBack(a);
                    PathDebugLines.EmplaceBack(PortalApex, PortalApex + n * Radius * sign * 2);

                    Vec2 b = PortalLeft + n * Radius;
                    path.Path.PushBack(b);
                    PathDebugLines.EmplaceBack(PortalLeft, PortalLeft + n * Radius * 2);

                    PortalApex = PortalLeft;
                    PortalLeft = PortalApex;
                    PortalRight = PortalApex;
                    PortalSide = 1;

                    ApexIndex = LeftIndex;
                    LeftIndex = ApexIndex;
                    RightIndex = ApexIndex;
                    ChainIndex = ApexIndex;
                    return false;
                }
            }

            if (TriArea2(PortalApex, PortalLeft, left) >= 0.0)
            {
                if (Vec2::Equals(PortalApex, PortalLeft) || TriArea2(PortalApex, PortalRight, left) < 0)
                {
                    // Tighten the funnel
                    PortalLeft = left;
                    LeftIndex = ChainIndex;
                }
                else
                {
                    Vec2 v = PortalApex - PortalRight;
                    Vec2 n = Vec2(v.Y, -v.X).Normalized();

                    auto sign = PortalSide == 2 ? 1 : -1;
                    Vec2 a = PortalApex + n * Radius * sign;
                    path.Path.PushBack(a);
                    PathDebugLines.EmplaceBack(PortalApex, PortalApex + n * Radius * sign * 2);

                    Vec2 b = PortalRight + n * Radius;
                    path.Path.PushBack(b);
                    PathDebugLines.EmplaceBack(PortalRight, PortalRight + n * Radius * 2);

                    PortalApex = PortalRight;
                    PortalLeft = PortalApex;
                    PortalRight = PortalApex;
                    PortalSide = 2;

                    ApexIndex = RightIndex;
                    LeftIndex = ApexIndex;
                    RightIndex = ApexIndex;
                    ChainIndex = ApexIndex;
                    return false;
                }
            }

            return false;
        }

        auto TriArea2(const Vec2& a, const Vec2& b, const Vec2& c)
        {
            auto av = b - a;
            auto bv = c - a;
            return av.Y * bv.X - av.X * bv.Y;
        }
    };

    
}
