
#pragma once

#include "PlatformTypes.h"
#include "Containers/FixedArray.h"

namespace Phoenix
{
    template <class TIdx = int16>
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
    };

    template <class TData, class TIdx = int16>
    struct TMeshFace
    {
        // The 3 half-edges that make up the face.
        TIdx HalfEdge = 0;

        // Some data associated with the face.
        TData Data = {};
    };

    template <size_t NFaces, class TFaceData, class TVert = Vec2, class TIdx = int16>
    struct TFixedMesh
    {
        using HalfEdge = TMeshHalfEdge<TIdx>;
        using Face = TMeshFace<TFaceData, TIdx>;
        static constexpr size_t HalfIdxBits = sizeof(TIdx) << 2;

        TIdx AddVertex(const TVert& v)
        {
            for (size_t i = 0; i < Vertices.Num(); ++i)
            {
                if (TVert::Equals(v, Vertices[i]))
                    return TIdx(i);
            }
            if (Vertices.IsFull())
                return INDEX_NONE;
            Vertices.Add(v);
            return TIdx(Vertices.Num()) - 1;
        }

        // Assumes that (a,b,c) is CCW
        TIdx AddFace(TIdx v0, TIdx v1, TIdx v2, const TFaceData& data)
        {
            TIdx f = (TIdx)Faces.Num();

            TIdx e0 = TIdx(HalfEdges.Num()) + 0;
            TIdx e1 = TIdx(HalfEdges.Num()) + 1;
            TIdx e2 = TIdx(HalfEdges.Num()) + 2;

            HalfEdge& edge0 = HalfEdges.AddDefaulted_GetRef();
            edge0.VertA = v0;
            edge0.VertB = v1;
            edge0.Twin = INDEX_NONE;
            edge0.Next = e1;
            edge0.Face = f;

            HalfEdge& edge1 = HalfEdges.AddDefaulted_GetRef();
            edge1.VertA = v1;
            edge1.VertB = v2;
            edge1.Twin = INDEX_NONE;
            edge1.Next = e2;
            edge1.Face = f;

            HalfEdge& edge2 = HalfEdges.AddDefaulted_GetRef();
            edge2.VertA = v2;
            edge2.VertB = v0;
            edge2.Twin = INDEX_NONE;
            edge2.Next = e0;
            edge2.Face = f;

            for (size_t i = 0; i < e0; ++i)
            {
                HalfEdge& e = HalfEdges[i];
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

            Faces.EmplaceBack(e0, data);

            return f;
        }

        // Assumes that (a,b,c) is CCW
        void AddFace(const TVert& a, const TVert& b, const TVert& c, const TFaceData& data)
        {
            TIdx v0 = AddVertex(a);
            TIdx v1 = AddVertex(b);
            TIdx v2 = AddVertex(c);
            AddFace(v0, v1, v2, data);
        }

        void RemoveFace(TIdx faceIndex)
        {
            auto& face = Faces[faceIndex];

            auto* e0 = &HalfEdges[face.HalfEdge];
            auto* e1 = &HalfEdges[e0->Next];
            auto* e2 = &HalfEdges[e1->Next];

            e0->Face = INDEX_NONE;
            e1->Face = INDEX_NONE;
            e2->Face = INDEX_NONE;

            if (Faces.Num() > 1)
            {
                TIdx lastFaceIdx = TIdx(Faces.Num()) - 1;
                auto& lastFace = Faces[lastFaceIdx];

                auto* le0 = &HalfEdges[lastFace.HalfEdge];
                auto* le1 = &HalfEdges[e0->Next];
                auto* le2 = &HalfEdges[e1->Next];

                le0->Face = faceIndex;
                le1->Face = faceIndex;
                le2->Face = faceIndex;
                *e0 = *le0;
                *e1 = *le1;
                *e2 = *le2;
                lastFace.HalfEdge = face.HalfEdge;

                Faces[faceIndex] = lastFace;
                Faces.SetNum(Faces.Num() - 1);
                HalfEdges.SetNum(HalfEdges.Num() - 3);
            }
        }

        void Reset()
        {
            Vertices.Reset();
            HalfEdges.Reset();
            Faces.Reset();
        }

        TFixedArray<TVert, NFaces*3> Vertices;
        TFixedArray<HalfEdge, NFaces*3> HalfEdges;
        TFixedArray<Face, NFaces> Faces;
    };

    // > 0 : inside
    // == 0 : co-circular
    // < 0 : outside
    auto PointInCircle(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& p)
    {
        auto ax = a - p;
        auto bx = b - p;
        auto cx = c - p;
        auto a2 = Vec2::Dot(ax, ax);
        auto b2 = Vec2::Dot(bx, bx);
        auto c2 = Vec2::Dot(cx, cx);
        auto d = a.X * (b.Y * c2 - c.Y * b2) -
               a.Y * (b.X * c2 - c.X * b2) +
               a2 * (b.X * c.Y - b.Y * c.X);
        return d;
    }

    auto Orient(const Vec2& a, const Vec2& b, const Vec2& p)
    {
        return (b.X - a.X) * (p.Y - a.Y) - (b.Y - a.Y) * (p.X - a.X);
    }

    auto PointInTriangle(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& p)
    {
        auto a0 = Orient(a, b, p);
        auto b0 = Orient(b, c, p);
        auto c0 = Orient(c, a, p);
        bool has_pos = (a0>0) || (b0>0) || (c0>0);
        bool has_neg = (a0<0) || (b0<0) || (c0<0);
        return !(has_pos && has_neg);
    }

    template <class T = TFixedMesh<8192, uint32, Vec2, int16>>
    void CDT_InsertPoint(T& mesh, const Vec2& v)
    {
        int16 vi = mesh.AddVertex(v);

        for (size_t i = 0; i < mesh.Faces.Num(); ++i)
        {
            const auto& face = mesh.Faces[i];

            const typename T::HalfEdge* e0 = &mesh.HalfEdges[face.HalfEdge];
            const typename T::HalfEdge* e1 = &mesh.HalfEdges[e0->Next];
            const typename T::HalfEdge* e2 = &mesh.HalfEdges[e1->Next];

            const Vec2& a = mesh.Vertices[e0->VertA];
            const Vec2& b = mesh.Vertices[e1->VertA];
            const Vec2& c = mesh.Vertices[e2->VertA];

            if (!PointInTriangle(a, b, c, v))
            {
                continue;
            }

            mesh.AddFace(vi, e0->VertA, e0->VertB, face.Data);
            mesh.AddFace(vi, e1->VertA, e1->VertB, face.Data);
            mesh.AddFace(vi, e2->VertA, e2->VertB, face.Data);
            mesh.RemoveFace(int16(i));

            break;
        }
    }
}
