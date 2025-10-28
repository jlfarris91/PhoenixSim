
#pragma once

#include "Mesh2.h"
#include "Containers/FixedQueue.h"
#include "Flags.h"
#include "Profiling.h"

#define MESH_TEMPLATE template <size_t NFaces, class TFaceData, class TVecComp, class TIdx>
#define MESH_CLASS TFixedCDTMesh2<NFaces, TFaceData, TVecComp, TIdx>

namespace Phoenix
{
    MESH_TEMPLATE
    void MESH_CLASS::Reset()
    {
        Vertices.Reset();
        HalfEdges.Reset();
        Faces.Reset();
    }

    MESH_TEMPLATE
    bool MESH_CLASS::IsValidVert(TIdx vertIndex) const
    {
        return Vertices.IsValidIndex(vertIndex);
    }

    MESH_TEMPLATE
    TIdx MESH_CLASS::InsertVertex(const TVec& pt, const TVecComp& threshold)
    {
        PHX_PROFILE_ZONE_SCOPED;

        for (size_t i = 0; i < Vertices.Num(); ++i)
        {
            if (TVec::Equals(pt, Vertices[i], threshold))
                return TIdx(i);
        }

        if (Vertices.IsFull())
        {
            return Index<TIdx>::None;
        }

        Vertices.EmplaceBack(pt);
        return TIdx(Vertices.Num()) - 1;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::SetVertex(TIdx vertIndex, const TVec& pt)
    {
        if (!IsValidVert(vertIndex))
            return false;
        Vertices[vertIndex] = pt;
        return true;
    }

    MESH_TEMPLATE
    const typename MESH_CLASS::TVec* MESH_CLASS::GetVertexPtr(TIdx vertIndex) const
    {
        return IsValidVert(vertIndex) ? &Vertices[vertIndex] : nullptr;
    }

    MESH_TEMPLATE
    const typename MESH_CLASS::TVec& MESH_CLASS::GetVertex(TIdx vertIndex) const
    {
        return *GetVertexPtr(vertIndex);
    }

    MESH_TEMPLATE
    TIdx MESH_CLASS::FindClosestVertex(const TVec& pt, const TOptional<TVecComp>& radius) const
    {
        TIdx idx = Index<TIdx>::None;
        TVecComp minDist = TVecComp::Max;
        for (size_t i = 0; i < Vertices.Num(); ++i)
        {
            auto dist = TVec::Distance(Vertices[i], pt);
            if ((!radius.IsSet() || dist < *radius) && dist < minDist)
            {
                minDist = dist;
                idx = TIdx(i);
            }
        }
        return idx;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::FindClosestVertices(
        const TVec& v0,
        const TVec& v1,
        TIdx& outV0,
        TIdx& outV1,
        const TVecComp& threshold) const
    {
        outV0 = Index<TIdx>::None;
        outV1 = Index<TIdx>::None;
        for (size_t i = 0; i < Vertices.Num(); ++i)
        {
            if (TVec::Equals(v0, Vertices[i], threshold))
                outV0 = TIdx(i);
            if (TVec::Equals(v1, Vertices[i], threshold))
                outV1 = TIdx(i);
            if (outV0 != Index<TIdx>::None && outV1 != Index<TIdx>::None)
                return true;
        }
        return false;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::IsVertLocked(TIdx vertIndex) const
    {
        if (!IsValidVert(vertIndex))
            return false;

        bool foundLocked = false;
        ForEachVertHalfEdge(vertIndex, [&foundLocked](const THalfEdge& halfEdge, TIdx)
        {
            if (halfEdge.bLocked)
            {
                foundLocked = true;
                return false;
            }
            return true;
        });

        return foundLocked;
    }

    MESH_TEMPLATE
    template <class T>
    void MESH_CLASS::ForEachVertInRange(const TVec& pos, TVecComp radius, T& callback) const
    {
        for (size_t i = 0; i < Vertices.Num(); ++i)
        {
            if (TVec::Distance(Vertices[i], pos) < radius)
            {
                if (!callback(Vertices[i], i))
                    return;
            }
        }
    }

    MESH_TEMPLATE
    template <class T>
    void MESH_CLASS::ForEachVertHalfEdge(TIdx vertIndex, T& callback, EHalfEdgeDirection direction) const
    {
        if (!IsValidVert(vertIndex))
            return;

        for (size_t i = 0; i < HalfEdges.Num(); ++i)
        {
            if (!IsValidHalfEdge(i))
                continue;

            const THalfEdge& halfEdge = HalfEdges[i];

            if ((HasAnyFlags(direction, EHalfEdgeDirection::Outgoing) && halfEdge.VertA == vertIndex) ||
                (HasAnyFlags(direction, EHalfEdgeDirection::Incoming) && halfEdge.VertB == vertIndex))
            {
                if (!callback(halfEdge, i))
                {
                    return;
                }
            }
        }
    }

    MESH_TEMPLATE
    bool MESH_CLASS::IsValidHalfEdge(TIdx halfEdgeIndex) const
    {
        return HalfEdges.IsValidIndex(halfEdgeIndex) && HalfEdges[halfEdgeIndex].Face != Index<TIdx>::None;
    }

    MESH_TEMPLATE
    const typename MESH_CLASS::THalfEdge* MESH_CLASS::GetHalfEdgePtr(TIdx halfEdgeIndex) const
    {
        return IsValidHalfEdge(halfEdgeIndex) ? &HalfEdges[halfEdgeIndex] : nullptr;
    }

    MESH_TEMPLATE
    const typename MESH_CLASS::THalfEdge& MESH_CLASS::GetHalfEdge(TIdx halfEdgeIndex) const
    {
        return *GetHalfEdgePtr(halfEdgeIndex);
    }

    MESH_TEMPLATE
    TIdx MESH_CLASS::InsertHalfEdge(TIdx vA, TIdx vB, TIdx f)
    {
        PHX_PROFILE_ZONE_SCOPED;

        TIdx e = Index<TIdx>::None;
        for (size_t i = 0; i < HalfEdges.Num(); ++i)
        {
            const THalfEdge& halfEdge = HalfEdges[i];
            if (e == Index<TIdx>::None && halfEdge.Face == Index<TIdx>::None)
            {
                e = TIdx(i);
            }
            // if (halfEdge.VertA == vA && halfEdge.VertB == vB)
            // {
            //     return TIdx(i);
            // }
        }

        if (e == Index<TIdx>::None)
        {
            e = TIdx(HalfEdges.Num());
            HalfEdges.AddDefaulted();
        }

        THalfEdge& edge = HalfEdges[e];
        edge.VertA = vA;
        edge.VertB = vB;
        edge.Twin = Index<TIdx>::None;
        edge.Next = Index<TIdx>::None;
        edge.Face = f;
        edge.bLocked = false;

        return e;
    }

    MESH_TEMPLATE
    TIdx MESH_CLASS::FindHalfEdge(
        const TVec& v0,
        const TVec& v1,
        TIdx& outV0,
        TIdx& outV1,
        const TVecComp& threshold) const
    {
        outV0 = Index<TIdx>::None;
        outV1 = Index<TIdx>::None;
        if (!FindClosestVertices(v0, v1, outV0, outV1, threshold))
        {
            return Index<TIdx>::None;
        }

        return FindHalfEdge(outV0, outV1);
    }

    MESH_TEMPLATE
    TIdx MESH_CLASS::FindHalfEdge(TIdx v0, TIdx v1) const
    {
        for (size_t i = 0; i < HalfEdges.Num(); ++i)
        {
            const THalfEdge& halfEdge = HalfEdges[i];
            if (halfEdge.Face == Index<TIdx>::None)
                continue;

            if (halfEdge.VertA == v0 && halfEdge.VertB == v1)
            {
                return TIdx(i);
            }
        }
        return Index<TIdx>::None;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::IsValidFace(TIdx faceIndex) const
    {
        return Faces.IsValidIndex(faceIndex) && IsValidHalfEdge(Faces[faceIndex].HalfEdge);
    }

    MESH_TEMPLATE
    const typename MESH_CLASS::TFace* MESH_CLASS::GetFacePtr(TIdx faceIndex) const
    {
        return IsValidFace(faceIndex) ? &Faces[faceIndex] : nullptr;
    }

    MESH_TEMPLATE
    const typename MESH_CLASS::TFace& MESH_CLASS::GetFace(TIdx faceIndex) const
    {
        return *GetFacePtr(faceIndex);
    }

    MESH_TEMPLATE
    bool MESH_CLASS::GetEdgeVerts(TIdx halfEdgeIndex, TVec& outVertA, TVec& outVertB) const
    {
        if (!IsValidHalfEdge(halfEdgeIndex))
            return false;

        const THalfEdge& halfEdge = HalfEdges[halfEdgeIndex];
        outVertA = Vertices[halfEdge.VertA];
        outVertB = Vertices[halfEdge.VertB];
        return true;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::GetEdgeCenter(TIdx halfEdgeIndex, TVec& outCenter) const
    {
        TVec vertA, vertB;
        if (!GetEdgeVerts(halfEdgeIndex, vertA, vertB))
            return false;
        outCenter = TVec::Midpoint(vertA, vertB);
        return true;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::GetEdgeCenterAndNormal(TIdx halfEdgeIndex, TVec& outCenter, TVec& outNormal) const
    {
        TVec vertA, vertB;
        if (!GetEdgeVerts(halfEdgeIndex, vertA, vertB))
            return false;
        outCenter = TVec::Midpoint(vertA, vertB);
        TVec n = (vertB - vertA).Normalized();
        outNormal = TVec(-n.Y, n.X);
        return true;
    }

    MESH_TEMPLATE
    auto MESH_CLASS::GetEdgeLength(TIdx halfEdgeIndex) const
    {
        TVec vertA, vertB;
        if (GetEdgeVerts(halfEdgeIndex, vertA, vertB))
        {
            return TVec::Distance(vertA, vertB);
        }
        return decltype(TVec::Distance(vertA, vertB)){};
    }

    MESH_TEMPLATE
    bool MESH_CLASS::IsEdgeLocked(TIdx halfEdgeIndex) const
    {
        if (!IsValidHalfEdge(halfEdgeIndex))
            return false;

        const THalfEdge& halfEdge = HalfEdges[halfEdgeIndex];
        if (halfEdge.bLocked)
            return true;

        if (!IsValidHalfEdge(halfEdge.Twin))
            return false;

        return HalfEdges[halfEdge.Twin].bLocked;
    }

    MESH_TEMPLATE
    TMeshEdge<TIdx> MESH_CLASS::FindEdge(
        const TVec& v0,
        const TVec& v1,
        TIdx& outV0,
        TIdx& outV1,
        const TVecComp& threshold) const
    {
        outV0 = Index<TIdx>::None;
        outV1 = Index<TIdx>::None;
        if (!FindClosestVertices(v0, v1, outV0, outV1, threshold))
        {
            return {};
        }

        return FindEdge(outV0, outV1);
    }

    MESH_TEMPLATE
    TMeshEdge<TIdx> MESH_CLASS::FindEdge(TIdx v0, TIdx v1) const
    {
        TMeshEdge<TIdx> result;
        uint8 count = 0;
        for (size_t i = 0; i < HalfEdges.Num() && count != 0x3; ++i)
        {
            const THalfEdge& halfEdge = HalfEdges[i];
            if (halfEdge.Face == Index<TIdx>::None)
                continue;

            if (halfEdge.VertA == v0 && halfEdge.VertB == v1)
            {
                result.HalfEdge0 = TIdx(i);
                count |= 0x1;
            }
            if (halfEdge.VertA == v1 && halfEdge.VertB == v0)
            {
                result.HalfEdge1 = TIdx(i);
                count |= 0x2;
            }
        }
        return result;
    }

    MESH_TEMPLATE
    TIdx MESH_CLASS::InsertFace(TIdx v0, TIdx v1, TIdx v2, const TFaceData& data)
    {
        PHX_PROFILE_ZONE_SCOPED;

        TIdx f = Index<TIdx>::None;

        for (size_t i = 0; i < Faces.Num(); ++i)
        {
            if (Faces[i].HalfEdge == Index<TIdx>::None)
            {
                f = TIdx(i);
                break;
            }
        }

        if (f == Index<TIdx>::None)
        {
            f = TIdx(Faces.Num());
            Faces.EmplaceBack(Index<TIdx>::None, data);
        }

        TIdx e0 = InsertHalfEdge(v0, v1, f);
        TIdx e1 = InsertHalfEdge(v1, v2, f);
        TIdx e2 = InsertHalfEdge(v2, v0, f);

        TFace& face = Faces[f];
        face.HalfEdge = e0;
        face.Data = data;

        THalfEdge& edge0 = HalfEdges[e0];
        edge0.Next = e1;

        THalfEdge& edge1 = HalfEdges[e1];
        edge1.Next = e2;

        THalfEdge& edge2 = HalfEdges[e2];
        edge2.Next = e0;

        for (size_t i = 0; i < HalfEdges.Num(); ++i)
        {
            THalfEdge& e = HalfEdges[i];

            if (e.Face == Index<TIdx>::None)
            {
                continue;
            }

            if (e.VertA == edge0.VertB && e.VertB == edge0.VertA)
            {
                edge0.Twin = TIdx(i);
                e.Twin = e0;
            }
            if (e.VertA == edge1.VertB && e.VertB == edge1.VertA)
            {
                edge1.Twin = TIdx(i);
                e.Twin = e1;
            }
            if (e.VertA == edge2.VertB && e.VertB == edge2.VertA)
            {
                edge2.Twin = TIdx(i);
                e.Twin = e2;
            }
        }

        return f;
    }

    MESH_TEMPLATE
    TIdx MESH_CLASS::InsertFace(const TVec& a, const TVec& b, const TVec& c, const TFaceData& data)
    {
        PHX_PROFILE_ZONE_SCOPED;

        TIdx v0 = InsertVertex(a);
        TIdx v1 = InsertVertex(b);
        TIdx v2 = InsertVertex(c);
        return InsertFace(v0, v1, v2, data);
    }

    MESH_TEMPLATE
    bool MESH_CLASS::RemoveFace(TIdx faceIndex)
    {
        if (!IsValidFace(faceIndex))
            return false;

        TFace& face = Faces[faceIndex];

        THalfEdge& e0 = HalfEdges[face.HalfEdge];
        THalfEdge& e1 = HalfEdges[e0.Next];
        THalfEdge& e2 = HalfEdges[e1.Next];

        // Invalidate edges and face
        e0.Face = Index<TIdx>::None;
        e1.Face = Index<TIdx>::None;
        e2.Face = Index<TIdx>::None;
        face.HalfEdge = Index<TIdx>::None;

        // Also remove any references to these edges from their twin edges
        if (IsValidHalfEdge(e0.Twin)) HalfEdges[e0.Twin].Twin = Index<TIdx>::None;
        if (IsValidHalfEdge(e1.Twin)) HalfEdges[e1.Twin].Twin = Index<TIdx>::None;
        if (IsValidHalfEdge(e2.Twin)) HalfEdges[e2.Twin].Twin = Index<TIdx>::None;

        return true;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::GetFaceVerts(TIdx faceIndex, TVec& outA, TVec& outB, TVec& outC) const
    {
        const TVec* ptrA, *ptrB, *ptrC;
        if (GetFaceVertPtrs(faceIndex, ptrA, ptrB, ptrC))
        {
            outA = *ptrA;
            outB = *ptrB;
            outC = *ptrC;
            return true;
        }
        return false;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::GetFaceVertPtrs(TIdx faceIndex, const TVec*& outA, const TVec*& outB, const TVec*& outC) const
    {
        if (!IsValidFace(faceIndex))
        {
            return false;
        }

        const TFace& face = Faces[faceIndex];
        TIdx edgeIndex;
        const THalfEdge* edge;

        // Vert A
        edgeIndex = face.HalfEdge;
        PHX_ASSERT(IsValidHalfEdge(edgeIndex));
        edge = &HalfEdges[edgeIndex];
        PHX_ASSERT(IsValidVert(edge->VertA));
        outA = &Vertices[edge->VertA];

        // Vert B
        edgeIndex = edge->Next;
        PHX_ASSERT(IsValidHalfEdge(edgeIndex));
        edge = &HalfEdges[edgeIndex];
        PHX_ASSERT(IsValidVert(edge->VertA));
        outB = &Vertices[edge->VertA];

        // Vert C
        edgeIndex = edge->Next;
        PHX_ASSERT(IsValidHalfEdge(edgeIndex));
        edge = &HalfEdges[edgeIndex];
        PHX_ASSERT(IsValidVert(edge->VertA));
        outC = &Vertices[edge->VertA];

        return true;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::GetFaceCenter(TIdx faceIndex, TVec& outCenter) const
    {
        TVec a, b, c;
        if (!GetFaceVerts(faceIndex, a, b, c))
        {
            return false;
        }

        outCenter = (a + b + c) / 3.0;
        return true;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::GetFaceArea(TIdx faceIndex, TVecComp& outArea) const
    {
        TVec a, b, c;
        if (!GetFaceVerts(faceIndex, a, b, c))
        {
            return false;
        }

        outArea = TriangleArea(a, b, c);
        return true;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::GetFaceBounds(TIdx faceIndex, TFixedBox<TVec>& outBounds) const
    {
        TVec* ptrA, ptrB, ptrC;
        if (GetFaceVertPtrs(faceIndex, ptrA, ptrB, ptrC))
        {
            outBounds = TFixedBox<TVec>::FromPoints(*ptrA, *ptrB, *ptrC);
            return true;
        }
        return false;
    }

    MESH_TEMPLATE
    TIdx MESH_CLASS::FindFaceContainingPoint(const TVec& pos) const
    {
        PHX_PROFILE_ZONE_SCOPED;

        for (size_t i = 0; i < Faces.Num(); ++i)
        {
            if (IsPointInFace(i, pos).Result == EPointInFaceResult::Inside)
            {
                return TIdx(i);
            }
        }
        return Index<TIdx>::None;
    }

    MESH_TEMPLATE
    PointInFaceResult<TIdx> MESH_CLASS::IsPointInFace(TIdx f, const TVec& p) const
    {
        PHX_PROFILE_ZONE_SCOPED;

        if (!IsValidFace(f))
        {
            return { EPointInFaceResult::Outside };
        }

        const TFace& face = Faces[f];

        const THalfEdge& edge0 = HalfEdges[face.HalfEdge];
        const THalfEdge& edge1 = HalfEdges[edge0.Next];
        const THalfEdge& edge2 = HalfEdges[edge1.Next];

        const TVec& a = Vertices[edge0.VertA];
        const TVec& b = Vertices[edge1.VertA];
        const TVec& c = Vertices[edge2.VertA];

        auto result = PointInTriangle<TVecComp, TIdx>(a, b, c, p);

        if (result.Result == EPointInFaceResult::OnEdge)
        {
            TIdx edgeIndices[] =
            {
                face.HalfEdge,
                edge0.Next,
                edge1.Next,
            };
            result.OnEdgeIndex = edgeIndices[result.OnEdgeIndex];
        }

        return result;
    }

    MESH_TEMPLATE
    void MESH_CLASS::SplitFace(
        TIdx faceIndex,
        TIdx vertIndex,
        TIdx& outFace0,
        TIdx& outFace1,
        TIdx& outFace2)
    {
        PHX_PROFILE_ZONE_SCOPED;

        // Split by inserting new faces and removing the old one
        {
            const TFace& face = Faces[faceIndex];
            THalfEdge edge0 = HalfEdges[face.HalfEdge];
            THalfEdge edge1 = HalfEdges[edge0.Next];
            THalfEdge edge2 = HalfEdges[edge1.Next];

            RemoveFace(faceIndex);
            outFace0 = InsertFace(vertIndex, edge0.VertA, edge0.VertB, face.Data);
            outFace1 = InsertFace(vertIndex, edge1.VertA, edge1.VertB, face.Data);
            outFace2 = InsertFace(vertIndex, edge2.VertA, edge2.VertB, face.Data);
        }
    }

    MESH_TEMPLATE
    void MESH_CLASS::SplitEdge(TIdx edgeIndex, TIdx vertIndex)
    {
        PHX_PROFILE_ZONE_SCOPED;

        PHX_ASSERT(IsValidHalfEdge(edgeIndex));
        const THalfEdge& edge = HalfEdges[edgeIndex];

        {
            PHX_ASSERT(IsValidFace(edge.Face));
            const TFace& face = Faces[edge.Face];

            const THalfEdge& edge1 = HalfEdges[edge.Next];
            const THalfEdge& edge2 = HalfEdges[edge1.Next];

            RemoveFace(edge.Face);
            InsertFace(vertIndex, edge1.VertA, edge1.VertB, face.Data);
            InsertFace(vertIndex, edge2.VertA, edge2.VertB, face.Data);
        }

        if (IsValidHalfEdge(edge.Twin))
        {
            const THalfEdge& twinEdge = HalfEdges[edge.Twin];

            PHX_ASSERT(IsValidFace(twinEdge.Face));
            const TFace& face = Faces[twinEdge.Face];

            const THalfEdge& edge1 = HalfEdges[twinEdge.Next];
            const THalfEdge& edge2 = HalfEdges[edge1.Next];

            RemoveFace(twinEdge.Face);
            InsertFace(vertIndex, edge1.VertA, edge1.VertB, face.Data);
            InsertFace(vertIndex, edge2.VertA, edge2.VertB, face.Data);
        }
    }

    MESH_TEMPLATE
    template <class TPredicate>
    void MESH_CLASS::ForEachHalfEdgeInFace(TIdx faceIndex, const TPredicate& pred) const
    {
        if (!IsValidFace(faceIndex))
            return;

        const TFace& face = Faces[faceIndex];

        TIdx edgeIndex = face.HalfEdge;

        for (size_t i = 0; i < 3; ++i)
        {
            PHX_ASSERT(IsValidHalfEdge(edgeIndex));
            const THalfEdge& halfEdge = HalfEdges[edgeIndex];
            edgeIndex = halfEdge.Next;
            pred(halfEdge);
        }
    }

    MESH_TEMPLATE
    template <class TPredicate>
    void MESH_CLASS::ForEachHalfEdgeIndexInFace(TIdx faceIndex, const TPredicate& pred) const
    {
        if (!IsValidFace(faceIndex))
            return;

        const TFace& face = Faces[faceIndex];

        TIdx edgeIndex = face.HalfEdge;

        for (size_t i = 0; i < 3; ++i)
        {
            PHX_ASSERT(IsValidHalfEdge(edgeIndex));
            pred(edgeIndex);
            const THalfEdge& halfEdge = HalfEdges[edgeIndex];
            edgeIndex = halfEdge.Next;
        }
    }

    MESH_TEMPLATE
    template <class TPredicate>
    void MESH_CLASS::ForEachHalfEdgeTwinInFace(TIdx faceIndex, const TPredicate& pred) const
    {
        if (!IsValidFace(faceIndex))
            return;

        const TFace& face = Faces[faceIndex];

        TIdx edgeIndex = face.HalfEdge;

        for (size_t i = 0; i < 3; ++i)
        {
            const THalfEdge& halfEdge = HalfEdges[edgeIndex];
            edgeIndex = halfEdge.Next;

            if (!IsValidHalfEdge(halfEdge.Twin))
                continue;

            const THalfEdge& twinEdge = HalfEdges[halfEdge.Twin];

            pred(twinEdge);
        }
    }

    MESH_TEMPLATE
    template <class TPredicate>
    void MESH_CLASS::ForEachNeighboringFaceIndex(TIdx faceIndex, const TPredicate& pred) const
    {
        if (!IsValidFace(faceIndex))
            return;

        const TFace& face = Faces[faceIndex];

        TIdx edgeIndex = face.HalfEdge;

        for (size_t i = 0; i < 3; ++i)
        {
            const THalfEdge& halfEdge = HalfEdges[edgeIndex];
            edgeIndex = halfEdge.Next;

            if (!IsValidHalfEdge(halfEdge.Twin))
                continue;

            const THalfEdge& twinEdge = HalfEdges[halfEdge.Twin];

            if (!IsValidFace(twinEdge.Face))
                continue;

            pred(twinEdge.Face);
        }
    }

    MESH_TEMPLATE
    template <class TPredicate>
    void MESH_CLASS::ForEachNeighboringFace(TIdx faceIndex, const TPredicate& pred) const
    {
        if (!IsValidFace(faceIndex))
            return;

        const TFace& face = Faces[faceIndex];

        TIdx edgeIndex = face.HalfEdge;

        for (size_t i = 0; i < 3; ++i)
        {
            const THalfEdge& halfEdge = HalfEdges[edgeIndex];
            edgeIndex = halfEdge.Next;

            if (!IsValidHalfEdge(halfEdge.Twin))
                continue;

            const THalfEdge& twinEdge = HalfEdges[halfEdge.Twin];

            if (!IsValidFace(twinEdge.Face))
                continue;

            const TFace& twinFace = Faces[twinEdge.Face];

            pred(twinFace);
        }
    }

    MESH_TEMPLATE
    void MESH_CLASS::FixDelaunayConditions(TIdx vi)
    {
        PHX_PROFILE_ZONE_SCOPED;

        TFixedQueue<int16, 128> stack;

        for (size_t i = 0; i < HalfEdges.Num(); ++i)
        {
            auto& edge = HalfEdges[i];
            if (edge.Face != Index<TIdx>::None && edge.VertA == vi && !edge.bLocked)
            {
                stack.Enqueue(TIdx(i));
            }
        }

        while (!stack.IsEmpty())
        {
            TIdx edgeIndex = stack.Dequeue();

            PHX_ASSERT(IsValidHalfEdge(edgeIndex));

            TIdx edgeIndex0 = edgeIndex;
            THalfEdge& edge0 = HalfEdges[edgeIndex0];

            TIdx edgeIndex1 = edge0.Next;
            PHX_ASSERT(IsValidHalfEdge(edgeIndex1));
            THalfEdge& edge1 = HalfEdges[edgeIndex1];

            TIdx edgeIndex2 = edge1.Next;
            PHX_ASSERT(IsValidHalfEdge(edgeIndex2));
            THalfEdge& edge2 = HalfEdges[edgeIndex2];

            TIdx twinEdgeIndex0 = edge1.Twin;
            if (!IsValidHalfEdge(edge1.Twin))
                continue;

            THalfEdge& twinEdge0 = HalfEdges[twinEdgeIndex0];
            if (twinEdge0.bLocked)
                continue;

            if (!IsValidFace(twinEdge0.Face))
                continue;

            TIdx twinEdgeIndex1 = twinEdge0.Next;
            PHX_ASSERT(IsValidHalfEdge(twinEdgeIndex1));
            THalfEdge& twinEdge1 = HalfEdges[twinEdgeIndex1];

            if (twinEdge1.bLocked)
                continue;

            TIdx twinEdgeIndex2 = twinEdge1.Next;
            PHX_ASSERT(IsValidHalfEdge(twinEdgeIndex2));
            THalfEdge& twinEdge2 = HalfEdges[twinEdgeIndex2];

            TIdx faceIndex = edge0.Face;
            PHX_ASSERT(IsValidFace(faceIndex));
            TFace& face = Faces[edge0.Face];

            TIdx twinFaceIndex = twinEdge0.Face;
            PHX_ASSERT(IsValidFace(twinFaceIndex));
            TFace& twinFace = Faces[twinFaceIndex];

            TIdx indexP = edge0.VertA; // P
            TIdx indexA = edge0.VertB;
            TIdx indexB = edge1.VertB;
            TIdx indexQ = twinEdge1.VertB;

            const TVec& p = Vertices[indexP];
            const TVec& a = Vertices[indexA];
            const TVec& b = Vertices[indexB];
            const TVec& q = Vertices[indexQ];

            if (PointInCircle(p, a, b, q) > 0)
            {
                edge1.VertA = indexQ;               // E1.A -> Q
                edge1.VertB = indexP;               // E1.B -> P
                edge1.Next = edgeIndex0;            // E1 -> E0
                edge0.Next = twinEdgeIndex1;        // E0 -> TE1

                twinEdge0.VertA = indexP;           // TE0.A -> P
                twinEdge0.VertB = indexQ;           // TE0.B -> Q
                twinEdge0.Next = twinEdgeIndex2;    // TE0 -> TE2
                twinEdge2.Next = edgeIndex2;        // TE2 -> E2

                // Assign new faces and twins
                edge2.Face = twinFaceIndex;         // E2.F = TF
                edge2.Next = twinEdgeIndex0;        // E2.N = TE0
                twinEdge1.Face = faceIndex;         // TE1.F = F
                twinEdge1.Next = edgeIndex1;        // TE1.N = E0

                if (face.HalfEdge == edgeIndex2)
                    face.HalfEdge = edgeIndex0;

                if (twinFace.HalfEdge == twinEdgeIndex1)
                    twinFace.HalfEdge = twinEdgeIndex0;

                stack.Enqueue(twinEdgeIndex0);
            }
        }
    }

    MESH_TEMPLATE
    void MESH_CLASS::TracePortalEdges(
        const auto& corridor,
        auto& outChainRhs,
        auto& outChainLhs,
        bool trimDuplicates) const
    {
        for (TIdx edgeIndex : corridor)
        {
            PHX_ASSERT(IsValidHalfEdge(edgeIndex));
            const THalfEdge& halfEdge = HalfEdges[edgeIndex];

            if (outChainLhs.IsEmpty() || !(trimDuplicates && outChainLhs.Back() == halfEdge.VertA))
            {
                outChainLhs.PushBack(halfEdge.VertA);
            }

            if (outChainRhs.IsEmpty() || !(trimDuplicates && outChainRhs.Back() == halfEdge.VertB))
            {
                outChainRhs.PushBack(halfEdge.VertB);
            }
        }
    }

    MESH_TEMPLATE
    void MESH_CLASS::TracePortalEdgeVerts(
        const auto& corridor,
        auto& outChainRhs,
        auto& outChainLhs) const
    {
        for (TIdx edgeIndex : corridor)
        {
            PHX_ASSERT(IsValidHalfEdge(edgeIndex));
            const THalfEdge& halfEdge = HalfEdges[edgeIndex];

            const TVec& vertA = Vertices[halfEdge.VertA];
            const TVec& vertB = Vertices[halfEdge.VertB];

            outChainLhs.PushBack(vertA);
            outChainRhs.PushBack(vertB);
        }
    }

    MESH_TEMPLATE
    void MESH_CLASS::TriangulatePolygon(auto& chain, const TFaceData& faceData)
    {
        PHX_PROFILE_ZONE_SCOPED;

        while (chain.Num() > 2)
        {
            int32 n = (int32)chain.Num();
            bool insertedFace = false;
            for (int32 i = 0; i < n; ++i)
            {
                TIdx i0 = (TIdx)Wrap(i - 1, 0, n);
                TIdx i1 = (TIdx)Wrap(i + 0, 0, n);
                TIdx i2 = (TIdx)Wrap(i + 1, 0, n);
                TIdx ptIdx0 = chain[i0];
                TIdx ptIdx1 = chain[i1];
                TIdx ptIdx2 = chain[i2];

                const TVec& pt0 = Vertices[ptIdx0];
                const TVec& pt1 = Vertices[ptIdx1];
                const TVec& pt2 = Vertices[ptIdx2];
                const TVec vpt10 = pt1 - pt0;
                const TVec vpt21 = pt2 - pt1;

                auto c = TVec::Cross(vpt10, vpt21);
                if (c <= 0)
                {
                    continue;
                }

                bool foundVertInside = false;
                for (int32 j = Wrap(i2 + 1, 0, n); j != i0; j = Wrap(j + 1, 0, n))
                {
                    TIdx ptIdxJ = chain[j];
                    const TVec& ptJ = Vertices[ptIdxJ];
                    PointInFaceResult<> result = PointInTriangle(pt0, pt1, pt2, ptJ);
                    if (result.Result == EPointInFaceResult::Inside)
                    {
                        foundVertInside = true;
                        break;
                    }
                    if (result.Result == EPointInFaceResult::OnEdge)
                    {
                        foundVertInside = true;
                        chain.RemoveAt(j);
                        break;
                    }
                }

                if (foundVertInside)
                    continue;

                InsertFace(ptIdx0, ptIdx1, ptIdx2, faceData);
                chain.RemoveAt(i);
                insertedFace = true;
                break;
            }

            if (!insertedFace)
                break;
        }
    }

    MESH_TEMPLATE
    TIdx MESH_CLASS::CDT_InsertPoint(const TVec& v, bool fixDelaunayConditions)
    {
        PHX_PROFILE_ZONE_SCOPED;

        TIdx containingFace = Index<TIdx>::None;
        TIdx containingEdge = Index<TIdx>::None;
        for (size_t i = 0; i < Faces.Num(); ++i)
        {
            auto faceIndex = TIdx(i);
            const auto& face = Faces[faceIndex];

            if (!IsValidHalfEdge(face.HalfEdge))
                continue;

            PointInFaceResult<TIdx> result = IsPointInFace(faceIndex, v);

            if (result.Result == EPointInFaceResult::Outside)
            {
                continue;
            }

            if (result.Result == EPointInFaceResult::OnEdge)
            {
                containingEdge = result.OnEdgeIndex;
                break;
            }

            if (result.Result == EPointInFaceResult::Inside)
            {
                containingFace = faceIndex;
                break;
            }
        }

        if (containingFace == Index<TIdx>::None && containingEdge == Index<TIdx>::None)
        {
            return Index<TIdx>::None;
        }

        TIdx vi = InsertVertex(v);

        if (containingFace != Index<TIdx>::None)
        {
            uint16 f0, f1, f2;
            SplitFace(containingFace, vi, f0, f1, f2);
        }
        else if (containingEdge != Index<TIdx>::None)
        {
            SplitEdge(containingEdge, vi);
        }
        else
        {
            return Index<TIdx>::None;
        }

        if (fixDelaunayConditions)
        {
            FixDelaunayConditions(vi);
        }

        return vi;
    }

    MESH_TEMPLATE
    bool MESH_CLASS::CDT_InsertEdge(const TLine<TVec>& line, bool fixDelaunayConditions)
    {
        PHX_PROFILE_ZONE_SCOPED;

        TIdx v0, v1;
        TMeshEdge<TIdx> edge = FindEdge(line.Start, line.End, v0, v1);

        // Edge already exists so just lock it.
        if (edge.HalfEdge0 != Index<TIdx>::None && edge.HalfEdge1 != Index<TIdx>::None)
        {
            HalfEdges[edge.HalfEdge0].bLocked = true;
            HalfEdges[edge.HalfEdge1].bLocked = true;
            return false;
        }

        if (v0 == Index<TIdx>::None)
        {
            v0 = CDT_InsertPoint(line.Start);
        }

        if (v1 == Index<TIdx>::None)
        {
            v1 = CDT_InsertPoint(line.End);
        }

        // Failed to insert one of the points
        if (v0 == Index<TIdx>::None || v1 == Index<TIdx>::None)
        {
            return false;
        }

        // Both verts of the line are the same vert
        if (v0 == v1)
        {
            return false;
        }

        // Get the verts again since they may have been snapped to another existing vert
        const TVec& vert0 = Vertices[v0];
        const TVec& vert1 = Vertices[v1];

        // TODO: how do we intelligently determine the sizes of these containers? 
        TFixedQueue<TIdx, 128> edgeQueue;
        TFixedArray<TIdx, 128> corridor;

        // Find all half-edges incident to the start vert of the line
        for (size_t i = 0; i < HalfEdges.Num(); ++i)
        {
            const THalfEdge& halfEdge = HalfEdges[i];
            if (halfEdge.Face != Index<TIdx>::None && halfEdge.VertA == v0)
            {
                edgeQueue.Enqueue(TIdx(i));
            }
        }

        if (edgeQueue.IsEmpty())
        {
            return false;
        }

        // Walk each triangle with an edge intersecting with the line
        bool done = false;
        while (!done && !edgeQueue.IsEmpty() && !corridor.IsFull())
        {
            TIdx edgeIndex = edgeQueue.Dequeue();

            if (!IsValidHalfEdge(edgeIndex))
                continue;

            THalfEdge& halfEdge0 = HalfEdges[edgeIndex];
            TIdx edgeIndexN = halfEdge0.Next;

            // Walk the half-edges to find one that intersects the line
            for (size_t i = 0; i < 2; ++i)
            {
                PHX_ASSERT(IsValidHalfEdge(edgeIndexN));
                const THalfEdge& halfEdgeN = HalfEdges[edgeIndexN];
                edgeIndexN = halfEdgeN.Next;

                if (halfEdgeN.VertA == v0 || halfEdgeN.VertB == v0)
                {
                    break;
                }

                // Done walking, we found the target vert
                if (halfEdgeN.VertA == v1 || halfEdgeN.VertB == v1)
                {
                    done = true;
                    corridor.PushBack(edgeIndex);
                    break;
                }

                const TVec& a = Vertices[halfEdgeN.VertA];
                const TVec& b = Vertices[halfEdgeN.VertB];

                TVec pt;
                if (TVec::Intersects(vert0, vert1, a, b, pt))
                {
                    edgeQueue.Enqueue(halfEdgeN.Twin);
                    corridor.PushBack(edgeIndex);
                    break;
                }
            }
        }

        if (corridor.Num() > 1)
        {
            TFixedArray<TIdx, 128> chainRhs;
            TFixedArray<TIdx, 128> chainLhs;

            // Add the first vertex to the corridor vert chains
            chainRhs.PushBack(v0);
            chainLhs.PushBack(v0);

            TracePortalEdges(corridor, chainRhs, chainLhs);

            // Add the last vert to the corridor vert chains
            chainRhs.PushBack(v1);
            chainLhs.PushBack(v1);

            // Reverse the lhs chain so that it's CCW
            std::reverse(chainLhs.begin(), chainLhs.end());

            // Remove faces
            for (TIdx edgeIndex : corridor)
            {
                RemoveFace(HalfEdges[edgeIndex].Face);
            }

            // Triangulate the rhs and lhs polygons of the corridor
            TriangulatePolygon(chainRhs, 0);
            TriangulatePolygon(chainLhs, 0);
        }

        auto lockedEdge = FindEdge(v0, v1);

        if (lockedEdge.HalfEdge0 != Index<TIdx>::None)
        {
            HalfEdges[lockedEdge.HalfEdge0].bLocked = true;
        }

        if (lockedEdge.HalfEdge1 != Index<TIdx>::None)
        {
            HalfEdges[lockedEdge.HalfEdge1].bLocked = true;
        }

        // PHX_ASSERT(lockedEdge.HalfEdge0 != Index<TIdx>::None);
        // PHX_ASSERT(lockedEdge.HalfEdge1 != Index<TIdx>::None);
        // HalfEdges[lockedEdge.HalfEdge0].bLocked = true;
        // HalfEdges[lockedEdge.HalfEdge1].bLocked = true;

        // Re-triangluate to respect delaunay
        if (fixDelaunayConditions)
        {
            FixDelaunayConditions(v0);
            FixDelaunayConditions(v1);
        }

        return true;
    }
}

#undef MESH_CLASS
#undef MESH_TEMPLATE