
#pragma once

#include "PlatformTypes.h"
#include "Containers/FixedArray.h"
#include "Containers/FixedQueue.h"

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
        TIdx HalfEdge1 = Index<TIdx>::None;
        TIdx HalfEdge2 = Index<TIdx>::None;
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
        
        // TVec2<float> af = { (float)a.X, (float)a.Y };
        // TVec2<float> bf = { (float)b.X, (float)b.Y };
        // TVec2<float> cf = { (float)c.X, (float)c.Y };
        // TVec2<float> pf = { (float)p.X, (float)p.Y };
        //
        // TVec2<float> daf = af - pf;
        // TVec2<float> dbf = bf - pf;
        // TVec2<float> dcf = cf - pf;
        // auto af2 = daf.X*daf.X + daf.Y*daf.Y;
        // auto bf2 = dbf.X*dbf.X + dbf.Y*dbf.Y;
        // auto cf2 = dcf.X*dcf.X + dcf.Y*dcf.Y;
        // int64 df0 = af.X * (bf.Y * cf2 - cf.Y * bf2);
        // int64 df1 = af.Y * (bf.X * cf2 - cf.X * bf2);
        // int64 df2 = af2 * (bf.X * cf.Y - bf.Y * cf.X);
        // auto df = df0 - df1 + df2;
        //
        // auto da = a - p;
        // auto db = b - p;
        // auto dc = c - p;
        // TFixed<16, int64> a2 = Vec2::Dot(da, da);
        // TFixed<16, int64> b2 = Vec2::Dot(db, db);
        // TFixed<16, int64> c2 = Vec2::Dot(dc, dc);
        // TFixed<16, int64> d0 = a.X * (b.Y * c2 - c.Y * b2);
        // TFixed<16, int64> d1 = a.Y * (b.X * c2 - c.X * b2);
        // TFixed<16, int64> d2 = a2 * (b.X * c.Y - b.Y * c.X);
        // TFixed<16, int64> d = d0 - d1 + d2;
        // return d;
    }

    template <size_t NFaces, class TFaceData, class TVert = Vec2, class TIdx = uint16>
    struct TFixedCDTMesh
    {
        using TIndex = TIdx;
        using THalfEdge = TMeshHalfEdge<TIdx>;
        using TFace = TMeshFace<TFaceData, TIdx>;
        using TVertComp = typename TVert::ComponentT;
        static constexpr TVertComp DefaultThreshold = 10.0;

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
                    result.HalfEdge1 = i;
                    count |= 0x1;
                }
                if (halfEdge.VertA == v1 && halfEdge.VertB == v0)
                {
                    result.HalfEdge2 = i;
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
            if (!Faces.IsValidIndex(faceIndex))
                return;

            TFace& face = Faces[faceIndex];

            if (!HalfEdges.IsValidIndex(face.HalfEdge))
                return;

            THalfEdge& e0 = HalfEdges[face.HalfEdge];
            THalfEdge& e1 = HalfEdges[e0.Next];
            THalfEdge& e2 = HalfEdges[e1.Next];

            // Invalidate edges and face
            e0.Face = Index<TIdx>::None;
            e1.Face = Index<TIdx>::None;
            e2.Face = Index<TIdx>::None;
            face.HalfEdge = Index<TIdx>::None;

            // Also remove any references to these edges from their twin edges
            if (HalfEdges.IsValidIndex(e0.Twin)) HalfEdges[e0.Twin].Twin = Index<TIdx>::None;
            if (HalfEdges.IsValidIndex(e1.Twin)) HalfEdges[e1.Twin].Twin = Index<TIdx>::None;
            if (HalfEdges.IsValidIndex(e2.Twin)) HalfEdges[e2.Twin].Twin = Index<TIdx>::None;
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
            PHX_ASSERT(HalfEdges.IsValidIndex(edgeIndex));
            const THalfEdge& edge = HalfEdges[edgeIndex];

            {
                PHX_ASSERT(Faces.IsValidIndex(edge.Face));
                const TFace& face = Faces[edge.Face];

                const THalfEdge& edge1 = HalfEdges[edge.Next];
                const THalfEdge& edge2 = HalfEdges[edge1.Next];

                InsertFace(vertIndex, edge1.VertA, edge1.VertB, face.Data);
                InsertFace(vertIndex, edge2.VertA, edge2.VertB, face.Data);
                RemoveFace(edge.Face);
            }

            if (HalfEdges.IsValidIndex(edge.Twin))
            {
                const THalfEdge& twinEdge = HalfEdges[edge.Twin];

                PHX_ASSERT(Faces.IsValidIndex(twinEdge.Face));
                const TFace& face = Faces[twinEdge.Face];

                const THalfEdge& edge1 = HalfEdges[twinEdge.Next];
                const THalfEdge& edge2 = HalfEdges[edge1.Next];

                InsertFace(vertIndex, edge1.VertA, edge1.VertB, face.Data);
                InsertFace(vertIndex, edge2.VertA, edge2.VertB, face.Data);
                RemoveFace(twinEdge.Face);
            }
        }

        PointInFaceResult<TIdx> PointInFace(TIdx f, const TVert& p)
        {
            if (!Faces.IsValidIndex(f))
            {
                return { EPointInFaceResult::Outside };
            }
            
            const TFace& face = Faces[f];

            if (!HalfEdges.IsValidIndex(face.HalfEdge))
            {
                return { EPointInFaceResult::Outside };
            }

            const THalfEdge& edge0 = HalfEdges[face.HalfEdge];
            const THalfEdge& edge1 = HalfEdges[edge0.Next];
            const THalfEdge& edge2 = HalfEdges[edge1.Next];

            TIdx edgeIdx0 = face.HalfEdge;
            TIdx edgeIdx1 = edge0.Next;
            TIdx edgeIdx2 = edge1.Next;

            const Vec2& a = Vertices[edge0.VertA];
            const Vec2& b = Vertices[edge1.VertA];
            const Vec2& c = Vertices[edge2.VertA];

            auto a0 = Orient(a, b, p);
            if (a0 == 0)
            {
                return { EPointInFaceResult::OnEdge, edgeIdx0 };
            }

            auto b0 = Orient(b, c, p);
            if (b0 == 0)
            {
                return { EPointInFaceResult::OnEdge, edgeIdx1 };
            }

            auto c0 = Orient(c, a, p);
            if (c0 == 0)
            {
                return { EPointInFaceResult::OnEdge, edgeIdx2 };
            }

            bool allPos = a0 > 0 && b0 > 0 && c0 > 0;
            bool allNeg = a0 < 0 && b0 < 0 && c0 < 0;
            if (allPos || allNeg)
            {
                return { EPointInFaceResult::Inside };
            }

            return { EPointInFaceResult::Outside };
        }

        void FixDelaunayConditions(TIdx vi)
        {
            TFixedQueue<int16, 64> stack;

            for (size_t i = 0; i < HalfEdges.Num(); ++i)
            {
                auto& edge = HalfEdges[i];
                if (edge.Face != Index<TIdx>::None && edge.VertA == vi)
                {
                    stack.Enqueue(TIdx(i));
                }
            }

            while (!stack.IsEmpty())
            {
                TIdx edgeIndex = stack.Dequeue();

                PHX_ASSERT(HalfEdges.IsValidIndex(edgeIndex));

                TIdx edgeIndex0 = edgeIndex;
                THalfEdge& edge0 = HalfEdges[edgeIndex0];

                TIdx edgeIndex1 = edge0.Next;
                PHX_ASSERT(HalfEdges.IsValidIndex(edgeIndex1));
                THalfEdge& edge1 = HalfEdges[edgeIndex1];

                TIdx edgeIndex2 = edge1.Next;
                PHX_ASSERT(HalfEdges.IsValidIndex(edgeIndex2));
                THalfEdge& edge2 = HalfEdges[edgeIndex2];

                TIdx twinEdgeIndex0 = edge1.Twin;
                if (!HalfEdges.IsValidIndex(edge1.Twin))
                    continue;

                THalfEdge& twinEdge0 = HalfEdges[twinEdgeIndex0];

                if (!Faces.IsValidIndex(twinEdge0.Face))
                    continue;

                TIdx twinEdgeIndex1 = twinEdge0.Next;
                PHX_ASSERT(HalfEdges.IsValidIndex(twinEdgeIndex1));
                THalfEdge& twinEdge1 = HalfEdges[twinEdgeIndex1];

                if (twinEdge1.bLocked)
                    continue;

                TIdx twinEdgeIndex2 = twinEdge1.Next;
                PHX_ASSERT(HalfEdges.IsValidIndex(twinEdgeIndex2));
                THalfEdge& twinEdge2 = HalfEdges[twinEdgeIndex2];

                TIdx faceIndex = edge0.Face;
                PHX_ASSERT(Faces.IsValidIndex(faceIndex));
                TFace& face = Faces[edge0.Face];

                TIdx twinFaceIndex = twinEdge0.Face;
                PHX_ASSERT(Faces.IsValidIndex(twinFaceIndex));
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

        void Reset()
        {
            Vertices.Reset();
            HalfEdges.Reset();
            Faces.Reset();
        }

        TFixedArray<TVert, NFaces*3> Vertices;
        TFixedArray<THalfEdge, NFaces*3> HalfEdges;
        TFixedArray<TFace, NFaces> Faces;
    };

    template <class T = TFixedCDTMesh<8192, uint32, Vec2, uint16>>
    typename T::TIndex CDT_InsertPoint(T& mesh, const Vec2& v)
    {
        using TIdx = typename T::TIndex;

        TIdx vi = mesh.InsertVertex(v);

        TIdx containingFace = Index<TIdx>::None;
        TIdx containingEdge = Index<TIdx>::None;
        for (size_t i = 0; i < mesh.Faces.Num(); ++i)
        {
            auto faceIndex = TIdx(i);
            const auto& face = mesh.Faces[faceIndex];

            if (!mesh.HalfEdges.IsValidIndex(face.HalfEdge))
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
            return vi;
        }

        mesh.FixDelaunayConditions(vi);

        return vi;
    }

    template <class T = TFixedCDTMesh<8192, uint32, Vec2, uint16>>
    void CDT_InsertEdge(T& mesh, const Line2& v)
    {
        using THalfEdge = typename T::THalfEdge;
        using TIdx = typename T::TIndex;

        TIdx v0, v1;
        TMeshEdge<TIdx> edge = mesh.FindEdge(v.Start, v.End, v0, v1);
        if (edge.HalfEdge1 != Index<TIdx>::None && edge.HalfEdge2 != Index<TIdx>::None)
        {
            mesh.HalfEdges[edge.HalfEdge1].bLocked = true;
            mesh.HalfEdges[edge.HalfEdge2].bLocked = true;
            return;
        }

        if (v0 == Index<TIdx>::None)
        {
            v0 = mesh.InsertVertex(v.Start);
        }

        if (v1 == Index<TIdx>::None)
        {
            // v1 = mesh.InsertVertex(v.End);
            v1 = CDT_InsertPoint(mesh, v.End);
        }

        // Get the verts again since they may have been snapped to another existing vert
        const Vec2& vertA = mesh.Vertices[v0];
        const Vec2& vertB = mesh.Vertices[v1];

        TFixedQueue<TIdx, 64> edgeStack;
        TFixedQueue<TIdx, 64> faceStack;
        TFixedArray<TIdx, 64> corridor;

        for (size_t i = 0; i < mesh.HalfEdges.Num(); ++i)
        {
            const THalfEdge& halfEdge = mesh.HalfEdges[i];
            if (halfEdge.Face != Index<TIdx>::None && halfEdge.VertA == v0)
            {
                edgeStack.Enqueue(TIdx(i));
            }
        }

        bool done = false;
        while (!done && !edgeStack.IsEmpty() && !faceStack.IsFull())
        {
            TIdx edgeIndex = edgeStack.Dequeue();

            if (!mesh.HalfEdges.IsValidIndex(edgeIndex))
                continue;

            THalfEdge& halfEdge0 = mesh.HalfEdges[edgeIndex];
            edgeIndex = halfEdge0.Next;

            for (size_t i = 0; i < 2; ++i)
            {
                PHX_ASSERT(mesh.HalfEdges.IsValidIndex(edgeIndex));
                const THalfEdge& halfEdgeN = mesh.HalfEdges[edgeIndex];

                if (halfEdgeN.VertA == v0 || halfEdgeN.VertB == v0)
                {
                    break;
                }

                // Done walking, we found the target vert
                if (halfEdgeN.VertA == v1 || halfEdgeN.VertB == v1)
                {
                    done = true;
                    faceStack.Enqueue(halfEdgeN.Face);
                    break;
                }

                const Vec2& a = mesh.Vertices[halfEdgeN.VertA];
                const Vec2& b = mesh.Vertices[halfEdgeN.VertB];

                Vec2 pt;
                if (Vec2::Intersects(vertA, vertB, a, b, pt))
                {
                    corridor.PushBack(halfEdgeN.VertA);
                    edgeStack.Enqueue(halfEdgeN.Twin);
                    faceStack.Enqueue(halfEdgeN.Face);
                    break;
                }

                edgeIndex = halfEdgeN.Next;
            }
        }

        if (faceStack.IsEmpty())
            return;

        while (!faceStack.IsEmpty())
        {
            TIdx faceIndex = faceStack.Dequeue();

            if (!mesh.Faces.IsValidIndex(faceIndex))
                continue;

            mesh.RemoveFace(faceIndex);
        }

        TIdx e0 = mesh.InsertHalfEdge(v0, v1, Index<TIdx>::None);
        TIdx e1 = mesh.InsertHalfEdge(v1, v0, Index<TIdx>::None);

        mesh.HalfEdges[e0].bLocked = true;
        mesh.HalfEdges[e1].bLocked = true;

        

        if (corridor.Num() > 1)
        {
            for (size_t i = 0; i < corridor.Num() - 2;)
            {
                const TIdx& va = corridor[i++];
                const TIdx& vb = corridor[i++];
                mesh.InsertFace(v0, va, vb, 100 + i);
            }
        }
    }
}
