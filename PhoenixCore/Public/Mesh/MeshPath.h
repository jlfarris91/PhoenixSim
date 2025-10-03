
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
            Value F;
            TIdx From;
            TVert Center;
            bool Visited = false;
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
        TIdx CurrFaceIndex = Index<TIdx>::None;
        uint32 Steps = 0;
        TFixedQueue<TIdx, 128> OpenSet;
        TFixedMap<TIdx, Node, 128> Nodes;
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
            CurrFaceIndex = Index<TIdx>::None;
            LastStepResult = EStepResult::Continue;

            if (StartFaceIndex == Index<TIdx>::None || GoalFaceIndex == Index<TIdx>::None)
            {
                return false;
            }

            OpenSet.Enqueue(StartFaceIndex);

            Node& startNode = FindOrAddNode(mesh, StartFaceIndex);
            startNode.G = 0;

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

            TIdx currFaceIndex = OpenSet.Dequeue();
            CurrFaceIndex = currFaceIndex;

            if (!mesh.IsValidFace(currFaceIndex))
            {
                LastStepResult = EStepResult::Continue;
                return LastStepResult;
            }

            // Found the goal triangle, we're done.
            if (currFaceIndex == GoalFaceIndex)
            {
                // TODO reconstruct triangle path
                LastStepResult = EStepResult::FoundPath;
                return LastStepResult;
            }

            Node& currNode = FindOrAddNode(mesh, currFaceIndex);
            currNode.Visited = true;

            {
                const TFace& face = mesh.Faces[currFaceIndex];

                TIdx edgeIndex = face.HalfEdge;

                for (size_t i = 0; i < 3; ++i)
                {
                    const THalfEdge& halfEdge = mesh.HalfEdges[edgeIndex];
                    edgeIndex = halfEdge.Next;

                    if (halfEdge.bLocked || !mesh.IsValidHalfEdge(halfEdge.Twin))
                        continue;

                    const THalfEdge& twinEdge = mesh.HalfEdges[halfEdge.Twin];

                    if (twinEdge.bLocked || !mesh.IsValidFace(twinEdge.Face))
                        continue;

                    auto neighborFaceIndex = twinEdge.Face;

                    {
                        Node& neighborNode = FindOrAddNode(mesh, neighborFaceIndex);

                        auto d = CalculateHeuristic(currFaceIndex, neighborFaceIndex);
                        auto score = currNode.G + d;
                        if (score < neighborNode.G)
                        {
                            neighborNode.From = currFaceIndex;
                            neighborNode.G = score;
                            neighborNode.F = score + CalculateHeuristicToGoal(neighborFaceIndex);

                            if (!neighborNode.Visited)
                                OpenSet.Enqueue(neighborFaceIndex);
                        }
                    }
                }
            }

            std::sort(OpenSet.begin(), OpenSet.end(), [&](TIdx a, TIdx b)
                {
                    return Nodes[a].F < Nodes[b].F;
                });

            LastStepResult = EStepResult::Continue;
            return LastStepResult;
        }

        Node& FindOrAddNode(const TMesh& mesh, TIdx faceIndex)
        {
            if (!Nodes.Contains(faceIndex))
            {
                Node& node = Nodes.InsertDefaulted_GetRef(faceIndex);
                node.Center = *mesh.GetFaceCenter(faceIndex);
                node.From = Index<TIdx>::None;
                node.G = Value::Max;
                node.F = CalculateHeuristicToGoal(faceIndex);
            }
            return Nodes[faceIndex];
        }

        Value CalculateHeuristicToGoal(TIdx faceIndex) const
        {
            return (GoalPos - Nodes[faceIndex].Center).Length();
        }

        Value CalculateHeuristic(TIdx currFaceIndex, TIdx neighborFaceIndex) const
        {
            return (Nodes[currFaceIndex].Center - Nodes[neighborFaceIndex].Center).Length();
        }

        void ResolvePath(TArray<TVert>& outPath)
        {
            TIdx idx = CurrFaceIndex;
            while (Nodes.Contains(idx))
            {
                Node& node = Nodes[idx];
                outPath.push_back(node.Center);
                idx = node.From;
            }
        }
        
    };


    
}
