
#pragma once

#include <algorithm>

#include "Mesh2.h"
#include "Containers/FixedQueue.h"
#include "Containers/FixedMap.h"

namespace Phoenix
{

    template <class TMesh = DefaultFixedCDTMesh2>
    struct TMeshPath
    {
        using TVert = typename TMesh::TVert;
        using TIdx = typename TMesh::TIndex;
        using THalfEdge = typename TMesh::THalfEdge;
        using TFace = typename TMesh::TFace;

        struct Node
        {
            Value G;
            Value H;
            TIdx FromEdge;
            TVert Center;
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
        TIdx StartFaceIndex = Index<TIdx>::None;
        TIdx GoalFaceIndex = Index<TIdx>::None;
        TIdx CurrEdgeIndex = Index<TIdx>::None;
        uint32 Steps = 0;
        TFixedQueue<TIdx, 1024> OpenSet;
        TFixedMap<TIdx, Node, 1024> Nodes;
        EStepResult LastStepResult = EStepResult::Continue;

        bool FindPath(const TMesh& mesh, const TVert& startPos, const TVert& goalPos, bool stepping)
        {
            StartPos = startPos;
            GoalPos = goalPos;
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

                OpenSet.Enqueue(halfEdgeIndex);

                Node& startNode = FindOrAddNode(mesh, halfEdgeIndex);
                startNode.G = CalculateHeuristicToStart(halfEdgeIndex);
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
                        Node& neighborNode = FindOrAddNode(mesh, twinEdgeIndex);

                        auto d = CalculateHeuristic(currEdgeIndex, twinEdgeIndex);
                        auto score = currNode.G + d;
                        if (score < neighborNode.G)
                        {
                            neighborNode.FromEdge = currEdgeIndex;
                            neighborNode.G = score;

                            if (!neighborNode.Visited)
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

        void ResolvePath(const TMesh& mesh, TArray<TVert>& outPath)
        {
            TIdx idx = CurrEdgeIndex;
            outPath.clear();
            outPath.push_back(GoalPos);
            while (Nodes.Contains(idx))
            {
                Node& node = Nodes[idx];
                outPath.push_back(node.Center);
                idx = node.FromEdge;
            }
            outPath.push_back(StartPos);
        }
        
    };


    
}
