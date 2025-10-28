
#pragma once

#include <algorithm>

#include "Mesh2.h"
#include "Containers/FixedQueue.h"
#include "Containers/FixedMap.h"
#include "Profiling.h"

namespace Phoenix
{
    template <class TMesh = DefaultFixedCDTMesh2>
    struct TMeshFunnel;

    template <class TMesh = DefaultFixedCDTMesh2>
    struct TMeshPath
    {
        using TVec = typename TMesh::TVec;
        using TVecComp = typename TMesh::TVecComp;
        using TIdx = typename TMesh::TIndex;
        using THalfEdge = typename TMesh::THalfEdge;
        using TFace = typename TMesh::TFace;

        struct Node
        {
            Value G;
            Value H;
            TIdx FromEdge = Index<TIdx>::None;
            TVec Center;
            bool Discarded = false;
            uint8 Visited = 0;
            Value GetF() const { return G + H; }
        };

        enum class EStepResult
        {
            Continue,
            FoundPath,
            Failed
        };

        TVec StartPos;
        TVec GoalPos;
        TVecComp Radius;
        TIdx StartFaceIndex = Index<TIdx>::None;
        TIdx GoalFaceIndex = Index<TIdx>::None;
        TIdx CurrEdgeIndex = Index<TIdx>::None;
        uint32 Steps = 0;
        TFixedQueue<TIdx, 4098> OpenSet;
        TFixedMap<TIdx, Node, 4098> Nodes;
        EStepResult LastStepResult = EStepResult::Continue;

        TFixedArray<TVec, 4098> Path;
        TFixedArray<TIdx, 4098> PathEdges;

        TMeshFunnel<TMesh> Funnel;

        bool FindPath(
            const TMesh& mesh,
            const TVec& startPos,
            const TVec& goalPos,
            TVecComp radius,
            bool stepping)
        {
            PHX_PROFILE_ZONE_SCOPED;

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
            PHX_PROFILE_ZONE_SCOPED;

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
            currNode.Visited = 2;

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
                    if (Nodes.Contains(twinHalfEdgeN.Twin) && Nodes[twinHalfEdgeN.Twin].Visited == 2)
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

                        if (neighborNode.Visited == 0)
                        {
                            neighborNode.Visited = 1;
                            OpenSet.Enqueue(twinEdgeIndex);
                        }
                    }

                    twinEdgeIndex = twinHalfEdgeN.Next;
                }
            }

            if (!OpenSet.IsEmpty())
            {
                std::sort(OpenSet.begin(), OpenSet.end(), [&](TIdx a, TIdx b)
                    { 
                        PHX_ASSERT(Nodes.Contains(a));
                        PHX_ASSERT(Nodes.Contains(b));
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
                node.Visited = 0;
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
            return TVec::Distance(GoalPos, Nodes[edgeIndex].Center);
        }

        Value CalculateHeuristicToStart(TIdx halfEdgeIndex) const
        {
            return TVec::Distance(StartPos, Nodes[halfEdgeIndex].Center);
        }

        Value CalculateHeuristic(TIdx currEdgeIndex, TIdx neighborEdgeIndex) const
        {
            return (Nodes[currEdgeIndex].Center - Nodes[neighborEdgeIndex].Center).Length();
        }

        void ResolvePath(const TMesh& mesh, bool stepping = false)
        {
            PHX_PROFILE_ZONE_SCOPED;

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
        using TVec = typename TMesh::TVec;
        using TVecComp = typename TMesh::TVecComp;
        using TIdx = typename TMesh::TIndex;
        using THalfEdge = typename TMesh::THalfEdge;
        using TFace = typename TMesh::TFace;

        TVec StartPos;
        TVec GoalPos;
        TVec PortalApex;
        TVec PortalLeft;
        TVec PortalRight;
        int32 ChainIndex = 0;
        int32 ApexIndex = 0;
        int32 LeftIndex = 0;
        int32 RightIndex = 0;
        int32 PortalSide = 0;
        Distance Radius;
        TFixedArray<TVec, 128> PathChainRhs;
        TFixedArray<TVec, 128> PathChainLhs;
        TFixedArray<TLine<TVec>, 128> PathDebugLines;

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
            PHX_PROFILE_ZONE_SCOPED;

            ++ChainIndex;

            if (!PathChainLhs.IsValidIndex(ChainIndex) || !PathChainRhs.IsValidIndex(ChainIndex))
            {
                return true;
            }

            const TVec& right = PathChainRhs[ChainIndex];
            const TVec& left = PathChainLhs[ChainIndex];

            if (TriangleArea(PortalApex, PortalRight, right) <= 0.0)
            {
                if (TVec::Equals(PortalApex, PortalRight) || TriangleArea(PortalApex, PortalLeft, right) > 0)
                {
                    // Tighten the funnel
                    PortalRight = right;
                    RightIndex = ChainIndex;
                }
                else
                {
                    TVec v = PortalApex - PortalLeft;
                    TVec n = TVec(-v.Y, v.X).Normalized();

                    auto sign = PortalSide == 1 ? 1 : -1;
                    TVec a = PortalApex + n * Radius * sign;
                    path.Path.PushBack(a);
                    PathDebugLines.EmplaceBack(PortalApex, PortalApex + n * Radius * sign * 2);

                    TVec b = PortalLeft + n * Radius;
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

            if (TriangleArea(PortalApex, PortalLeft, left) >= 0.0)
            {
                if (TVec::Equals(PortalApex, PortalLeft) || TriangleArea(PortalApex, PortalRight, left) < 0)
                {
                    // Tighten the funnel
                    PortalLeft = left;
                    LeftIndex = ChainIndex;
                }
                else
                {
                    TVec v = PortalApex - PortalRight;
                    TVec n = TVec(v.Y, -v.X).Normalized();

                    auto sign = PortalSide == 2 ? 1 : -1;
                    TVec a = PortalApex + n * Radius * sign;
                    path.Path.PushBack(a);
                    PathDebugLines.EmplaceBack(PortalApex, PortalApex + n * Radius * sign * 2);

                    TVec b = PortalRight + n * Radius;
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
    };
}
