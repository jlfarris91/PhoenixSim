
#pragma once

#include "Optional.h"
#include "PlatformTypes.h"
#include "Containers/FixedArray.h"
#include "Containers/FixedQueue.h"
#include "FixedPoint/FixedMath.h"

namespace Phoenix
{
    template <class TIdx = uint16>
    struct TMeshHalfEdge
    {
        // Vertex index at the "end" of the half-edge.
        TIdx VertA = 0;
        TIdx VertB = 0;

        // The opposite half-edge.
        TIdx Twin = 0;

        // The next half-edge CCW around the face.
        TIdx Next = 0;

        // The owning face id.
        TIdx Face = 0;

        uint8 bLocked = 0;
    };

    template <class TIdx = uint16>
    struct TMeshEdge
    {
        TIdx HalfEdge0 = Index<TIdx>::None;
        TIdx HalfEdge1 = Index<TIdx>::None;
    };

    template <class TData, class TIdx = uint16>
    struct TMeshFace
    {
        // The 3 half-edges that make up the face.
        TIdx HalfEdge = 0;

        // Some data associated with the face.
        TData Data = {};
    };

    enum class EPointInFaceResult
    {
        Outside,
        Inside,
        OnEdge
    };

    template <class TIdx>
    struct PointInFaceResult
    {
        EPointInFaceResult Result;
        TIdx OnEdgeIndex = 0;
    };

    auto Orient(const Vec2& a, const Vec2& b, const Vec2& p)
    {
        return (b.X - a.X) * (p.Y - a.Y) - (b.Y - a.Y) * (p.X - a.X);
    }

    // > 0 : inside
    // == 0 : co-circular
    // < 0 : outside
    auto PointInCircle(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& p)
    {
        auto a2 = a.X*a.X + a.Y*a.Y;
        auto b2 = b.X*b.X + b.Y*b.Y;
        auto c2 = c.X*c.X + c.Y*c.Y;
        
        auto d = 2 * (a.X * (b.Y - c.Y) + b.X * (c.Y - a.Y) + c.X * (a.Y - b.Y));
        if (d == 0)
            return TFixed<16, int64>(0);
        
        auto ux = (a2 * (b.Y - c.Y) + b2 * (c.Y - a.Y) + c2 * (a.Y - b.Y)) / d;
        auto uy = (a2 * (c.X - b.X) + b2 * (a.X - c.X) + c2 * (b.X - a.X)) / d;
        auto r1 = (ux - a.X)*(ux - a.X) + (uy - a.Y)*(uy - a.Y);

        auto v = p - Vec2(ux, uy);
        auto asdf = Vec2::Dot(v, v);
        auto asdfsdf = r1 - asdf;

        return asdfsdf;
    }

    template <class TIdx = int32>
    PointInFaceResult<TIdx> PointInTriangle(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& p)
    {
        auto a0 = Orient(a, b, p);
        if (a0 == 0)
        {
            return { EPointInFaceResult::OnEdge, TIdx(0) };
        }

        auto b0 = Orient(b, c, p);
        if (b0 == 0)
        {
            return { EPointInFaceResult::OnEdge, TIdx(1) };
        }

        auto c0 = Orient(c, a, p);
        if (c0 == 0)
        {
            return { EPointInFaceResult::OnEdge, TIdx(2) };
        }

        bool allPos = a0 > 0 && b0 > 0 && c0 > 0;
        bool allNeg = a0 < 0 && b0 < 0 && c0 < 0;
        if (allPos || allNeg)
        {
            return { EPointInFaceResult::Inside };
        }

        return { EPointInFaceResult::Outside };
    }

    auto TriangulatePolygon(auto& mesh, auto& chain)
    {
        while (chain.Num() > 2)
        {
            int32 n = chain.Num();
            bool insertedFace = false;
            for (int32 i = 0; i < n; ++i)
            {
                auto i0 = Wrap(i - 1, 0, n);
                auto i1 = Wrap(i + 0, 0, n);
                auto i2 = Wrap(i + 1, 0, n);
                auto ptIdx0 = chain[i0];
                auto ptIdx1 = chain[i1];
                auto ptIdx2 = chain[i2];

                const Vec2& pt0 = mesh.Vertices[ptIdx0];
                const Vec2& pt1 = mesh.Vertices[ptIdx1];
                const Vec2& pt2 = mesh.Vertices[ptIdx2];
                const Vec2 vpt10 = pt1 - pt0;
                const Vec2 vpt21 = pt2 - pt1;

                auto c = Vec2::Cross(vpt10, vpt21);
                if (c <= 0)
                {
                    continue;
                }

                bool foundVertInside = false;
                for (int32 j = Wrap(i2 + 1, 0, n); j != i0; j = Wrap(j + 1, 0, n))
                {
                    auto ptIdxJ = chain[j];
                    const Vec2& ptJ = mesh.Vertices[ptIdxJ];
                    auto result = PointInTriangle(pt0, pt1, pt2, ptJ);
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

                mesh.InsertFace(ptIdx0, ptIdx1, ptIdx2, 100 + i);
                chain.RemoveAt(i);
                insertedFace = true;
                break;
            }

            if (!insertedFace)
                break;
        }
    }

    template <size_t NFaces, class TFaceData, class TVecComp = Distance, class TIdx = uint16>
    struct TFixedCDTMesh2
    {
        using TIndex = TIdx;
        using THalfEdge = TMeshHalfEdge<TIdx>;
        using TFace = TMeshFace<TFaceData, TIdx>;
        using TVert = TVec2<TVecComp>;
        using TVertComp = typename TVert::ComponentT;
        static constexpr TVertComp DefaultThreshold = 10.0;

        bool IsValidVert(TIdx vertIndex) const
        {
            return Vertices.IsValidIndex(vertIndex);
        }

        bool IsValidHalfEdge(TIdx halfEdgeIndex) const
        {
            return HalfEdges.IsValidIndex(halfEdgeIndex) && HalfEdges[halfEdgeIndex].Face != Index<TIdx>::None;
        }

        bool IsValidFace(TIdx faceIndex) const
        {
            return Faces.IsValidIndex(faceIndex) && IsValidHalfEdge(Faces[faceIndex].HalfEdge);
        }

        bool FindVertsForEdge(
            const TVert& v0,
            const TVert& v1,
            TIdx& outV0,
            TIdx& outV1,
            const TVertComp& threshold = DefaultThreshold) const
        {
            outV0 = Index<TIdx>::None;
            outV1 = Index<TIdx>::None;
            for (size_t i = 0; i < Vertices.Num(); ++i)
            {
                if (TVert::Equals(v0, Vertices[i], threshold))
                    outV0 = TIdx(i);
                if (TVert::Equals(v1, Vertices[i], threshold))
                    outV1 = TIdx(i);
                if (outV0 != Index<TIdx>::None && outV1 != Index<TIdx>::None)
                    return true;
            }
            return false;
        }

        bool GetEdgeVerts(TIdx edgeIndex, TVert& outVertA, TVert& outVertB) const
        {
            if (!IsValidHalfEdge(edgeIndex))
                return false;

            const THalfEdge& halfEdge = HalfEdges[edgeIndex];
            outVertA = Vertices[halfEdge.VertA];
            outVertB = Vertices[halfEdge.VertB];
            return true;
        }

        bool GetEdgeCenter(TIdx edgeIndex, TVert& outCenter) const
        {
            TVert vertA, vertB;
            if (!GetEdgeVerts(edgeIndex, vertA, vertB))
                return false;
            outCenter = Vec2::Midpoint(vertA, vertB);
            return true;
        }

        TIdx InsertVertex(const TVert& v, const TVertComp& threshold = DefaultThreshold)
        {
            for (size_t i = 0; i < Vertices.Num(); ++i)
            {
                if (TVert::Equals(v, Vertices[i], threshold))
                    return TIdx(i);
            }

            if (Vertices.IsFull())
            {
                return Index<TIdx>::None;
            }

            Vertices.EmplaceBack(v);
            return TIdx(Vertices.Num()) - 1;
        }

        TIdx InsertHalfEdge(TIdx vA, TIdx vB, TIdx f)
        {
            TIdx e = Index<TIdx>::None;
            for (size_t i = 0; i < HalfEdges.Num(); ++i)
            {
                if (HalfEdges[i].Face == Index<TIdx>::None)
                {
                    e = TIdx(i);
                    break;
                }
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

        TIdx FindHalfEdge(
            const TVert& v0,
            const TVert& v1,
            TIdx& outV0,
            TIdx& outV1,
            const TVertComp& threshold = DefaultThreshold) const
        {
            outV0 = Index<TIdx>::None;
            outV1 = Index<TIdx>::None;
            if (!FindVertsForEdge(v0, v1, outV0, outV1, threshold))
            {
                return Index<TIdx>::None;
            }

            return FindHalfEdge(outV0, outV1);
        }

        TIdx FindHalfEdge(TIdx v0, TIdx v1) const
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

        TMeshEdge<TIdx> FindEdge(
            const TVert& v0,
            const TVert& v1,
            TIdx& outV0,
            TIdx& outV1,
            const TVertComp& threshold = DefaultThreshold) const
        {
            outV0 = Index<TIdx>::None;
            outV1 = Index<TIdx>::None;
            if (!FindVertsForEdge(v0, v1, outV0, outV1, threshold))
            {
                return {};
            }

            return FindEdge(outV0, outV1);
        }

        TMeshEdge<TIdx> FindEdge(TIdx v0, TIdx v1) const
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

        // Assumes that (a,b,c) is CCW
        TIdx InsertFace(TIdx v0, TIdx v1, TIdx v2, const TFaceData& data)
        {
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

        // Assumes that (a,b,c) is CCW
        void InsertFace(const TVert& a, const TVert& b, const TVert& c, const TFaceData& data)
        {
            TIdx v0 = InsertVertex(a);
            TIdx v1 = InsertVertex(b);
            TIdx v2 = InsertVertex(c);
            InsertFace(v0, v1, v2, data);
        }

        void RemoveFace(TIdx faceIndex)
        {
            if (!IsValidFace(faceIndex))
                return;

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
        }

        void SplitFace(TIdx faceIndex, TIdx vertIndex, TIdx& outFace0, TIdx& outFace1, TIdx& outFace2)
        {
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

        void SplitEdge(TIdx edgeIndex, TIdx vertIndex)
        {
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

        PointInFaceResult<TIdx> PointInFace(TIdx f, const TVert& p) const
        {
            if (!IsValidFace(f))
            {
                return { EPointInFaceResult::Outside };
            }

            const TFace& face = Faces[f];

            const THalfEdge& edge0 = HalfEdges[face.HalfEdge];
            const THalfEdge& edge1 = HalfEdges[edge0.Next];
            const THalfEdge& edge2 = HalfEdges[edge1.Next];

            const Vec2& a = Vertices[edge0.VertA];
            const Vec2& b = Vertices[edge1.VertA];
            const Vec2& c = Vertices[edge2.VertA];

            auto result = PointInTriangle<TIdx>(a, b, c, p);

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

        template <class TPredicate>
        void ForEachHalfEdgeInFace(TIdx faceIndex, const TPredicate& pred) const
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

        template <class TPredicate>
        void ForEachHalfEdgeIndexInFace(TIdx faceIndex, const TPredicate& pred) const
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

        template <class TPredicate>
        void ForEachHalfEdgeTwinInFace(TIdx faceIndex, const TPredicate& pred) const
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

        template <class TPredicate>
        void ForEachNeighboringFaceIndex(TIdx faceIndex, const TPredicate& pred) const
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

        template <class TPredicate>
        void ForEachNeighboringFace(TIdx faceIndex, const TPredicate& pred) const
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

        // Gets the index of the face containing the point or Index<TIdx>::None.
        TIdx FindFaceContainingPoint(const TVert& pos) const
        {
            for (size_t i = 0; i < Faces.Num(); ++i)
            {
                if (PointInFace(i, pos).Result == EPointInFaceResult::Inside)
                {
                    return TIdx(i);
                }
            }
            return Index<TIdx>::None;
        }

        bool GetFaceVerts(TIdx faceIndex, TVert& outA, TVert& outB, TVert& outC) const
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
            outA = Vertices[edge->VertA];

            // Vert B
            edgeIndex = edge->Next;
            PHX_ASSERT(IsValidHalfEdge(edgeIndex));
            edge = &HalfEdges[edgeIndex];
            PHX_ASSERT(IsValidVert(edge->VertA));
            outB = Vertices[edge->VertA];

            // Vert C
            edgeIndex = edge->Next;
            PHX_ASSERT(IsValidHalfEdge(edgeIndex));
            edge = &HalfEdges[edgeIndex];
            PHX_ASSERT(IsValidVert(edge->VertA));
            outC = Vertices[edge->VertA];

            return true;
        }

        TOptional<TVert> GetFaceCenter(TIdx faceIndex) const
        {
            if (!IsValidFace(faceIndex))
            {
                return {};
            }

            TVert a, b, c;
            if (!GetFaceVerts(faceIndex, a, b, c))
            {
                return {};
            }

            TVert center = (a + b + c) / 3.0;
            return { center };
        }

        void FixDelaunayConditions(TIdx vi)
        {
            TFixedQueue<int16, 64> stack;

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

                const Vec2& p = Vertices[indexP];
                const Vec2& a = Vertices[indexA];
                const Vec2& b = Vertices[indexB];
                const Vec2& q = Vertices[indexQ];

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

        // Returns true if either half-edge of a given edge is locked.
        // TODO (jfarris): IF ONE IS LOCKED THEN THEY BOTH SHOULD BE! but they aren't always for some reason...
        bool IsEdgeLocked(TIdx halfEdgeIndex) const
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

        void Reset()
        {
            Vertices.Reset();
            HalfEdges.Reset();
            Faces.Reset();
        }

        void TracePortalEdges(const auto& corridor, auto& outChainRhs, auto& outChainLhs)
        {
            for (TIdx edgeIndex : corridor)
            {
                PHX_ASSERT(IsValidHalfEdge(edgeIndex));
                const THalfEdge& halfEdge = HalfEdges[edgeIndex];

                if (outChainLhs.IsEmpty() || outChainLhs.Back() != halfEdge.VertA)
                {
                    outChainLhs.PushBack(halfEdge.VertA);
                }

                if (outChainRhs.IsEmpty() || outChainRhs.Back() != halfEdge.VertB)
                {
                    outChainRhs.PushBack(halfEdge.VertB);
                }
            }
        }

        TFixedArray<TVert, NFaces*3> Vertices;
        TFixedArray<THalfEdge, NFaces*3> HalfEdges;
        TFixedArray<TFace, NFaces> Faces;
    };

    using DefaultFixedCDTMesh2 = TFixedCDTMesh2<8192, uint32, Distance, uint16>;

    template <class T = DefaultFixedCDTMesh2>
    typename T::TIndex CDT_InsertPoint(T& mesh, const Vec2& v)
    {
        using TIdx = typename T::TIndex;

        TIdx containingFace = Index<TIdx>::None;
        TIdx containingEdge = Index<TIdx>::None;
        for (size_t i = 0; i < mesh.Faces.Num(); ++i)
        {
            auto faceIndex = TIdx(i);
            const auto& face = mesh.Faces[faceIndex];

            if (!mesh.IsValidHalfEdge(face.HalfEdge))
                continue;

            PointInFaceResult<TIdx> result = mesh.PointInFace(faceIndex, v);

            if (result.Result == EPointInFaceResult::Outside)
            {
                continue;
            }

            if (result.Result == EPointInFaceResult::Inside)
            {
                containingFace = faceIndex;
            }
            else if (result.Result == EPointInFaceResult::OnEdge)
            {
                // containingEdge = result.OnEdgeIndex;
                continue;
            }
        }

        if (containingFace == Index<TIdx>::None && containingEdge == Index<TIdx>::None)
        {
            return Index<TIdx>::None;
        }

        TIdx vi = mesh.InsertVertex(v);

        if (containingFace != Index<TIdx>::None)
        {
            uint16 f0, f1, f2;
            mesh.SplitFace(containingFace, vi, f0, f1, f2);
        }
        else if (containingEdge != Index<TIdx>::None)
        {
            mesh.SplitEdge(containingEdge, vi);
        }
        else
        {
            return Index<TIdx>::None;
        }

        mesh.FixDelaunayConditions(vi);

        return vi;
    }

    template <class T = DefaultFixedCDTMesh2>
    void CDT_InsertEdge(T& mesh, const Line2& v)
    {
        using THalfEdge = typename T::THalfEdge;
        using TIdx = typename T::TIndex;

        TIdx v0, v1;
        TMeshEdge<TIdx> edge = mesh.FindEdge(v.Start, v.End, v0, v1);

        // Edge already exists so just lock it.
        if (edge.HalfEdge0 != Index<TIdx>::None && edge.HalfEdge1 != Index<TIdx>::None)
        {
            mesh.HalfEdges[edge.HalfEdge0].bLocked = true;
            mesh.HalfEdges[edge.HalfEdge1].bLocked = true;
            return;
        }

        if (v0 == Index<TIdx>::None)
        {
            v0 = CDT_InsertPoint(mesh, v.Start);
        }

        if (v1 == Index<TIdx>::None)
        {
            v1 = CDT_InsertPoint(mesh, v.End);
        }

        // Failed to insert one of the points
        if (v0 == Index<TIdx>::None || v1 == Index<TIdx>::None)
        {
            return;
        }

        // Both verts of the line are the same vert
        if (v0 == v1)
        {
            return;
        }

        // Get the verts again since they may have been snapped to another existing vert
        const Vec2& vert0 = mesh.Vertices[v0];
        const Vec2& vert1 = mesh.Vertices[v1];

        // TODO: how do we intelligently determine the sizes of these containers? 
        TFixedQueue<TIdx, 128> edgeQueue;
        TFixedArray<TIdx, 128> corridor;

        // Find all half-edges incident to the start vert of the line
        for (size_t i = 0; i < mesh.HalfEdges.Num(); ++i)
        {
            const THalfEdge& halfEdge = mesh.HalfEdges[i];
            if (halfEdge.Face != Index<TIdx>::None && halfEdge.VertA == v0)
            {
                edgeQueue.Enqueue(TIdx(i));
            }
        }

        if (edgeQueue.IsEmpty())
            return;

        // Walk each triangle with an edge intersecting with the line
        bool done = false;
        while (!done && !edgeQueue.IsEmpty() && !corridor.IsFull())
        {
            TIdx edgeIndex = edgeQueue.Dequeue();

            if (!mesh.IsValidHalfEdge(edgeIndex))
                continue;

            THalfEdge& halfEdge0 = mesh.HalfEdges[edgeIndex];
            TIdx edgeIndexN = halfEdge0.Next;

            // Walk the half-edges to find one that intersects the line
            for (size_t i = 0; i < 2; ++i)
            {
                PHX_ASSERT(mesh.IsValidHalfEdge(edgeIndexN));
                const THalfEdge& halfEdgeN = mesh.HalfEdges[edgeIndexN];
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

                const Vec2& a = mesh.Vertices[halfEdgeN.VertA];
                const Vec2& b = mesh.Vertices[halfEdgeN.VertB];

                Vec2 pt;
                if (Vec2::Intersects(vert0, vert1, a, b, pt))
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

            mesh.TracePortalEdges(corridor, chainRhs, chainLhs);

            // Add the last vert to the corridor vert chains
            chainRhs.PushBack(v1);
            chainLhs.PushBack(v1);

            // Reverse the lhs chain so that it's CCW
            std::reverse(chainLhs.begin(), chainLhs.end());

            // Remove faces
            for (TIdx edgeIndex : corridor)
            {
                mesh.RemoveFace(mesh.HalfEdges[edgeIndex].Face);
            }

            // Triangulate the rhs and lhs polygons of the corridor
            TriangulatePolygon(mesh, chainRhs);
            TriangulatePolygon(mesh, chainLhs);
        }

        auto lockedEdge = mesh.FindEdge(v0, v1);

        PHX_ASSERT(lockedEdge.HalfEdge0 != Index<TIdx>::None);
        PHX_ASSERT(lockedEdge.HalfEdge1 != Index<TIdx>::None);
        mesh.HalfEdges[lockedEdge.HalfEdge0].bLocked = true;
        mesh.HalfEdges[lockedEdge.HalfEdge1].bLocked = true;

        // Re-triangluate to respect delaunay
        mesh.FixDelaunayConditions(v0);
        mesh.FixDelaunayConditions(v1);
    }
}