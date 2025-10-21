
#pragma once

#include "FixedBVH.h"
#include "Optional.h"
#include "PlatformTypes.h"
#include "Containers/FixedArray.h"
#include "FixedPoint/FixedMath.h"
#include "FixedPoint/FixedVector.h"
#include "FixedPoint/FixedLine.h"

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

    enum class EHalfEdgeDirection : uint8
    {
        Incoming = 1,
        Outgoing = 2,
        Both = Incoming | Outgoing
    };

    template <size_t NFaces, class TFaceData, class TVecComp_ = Distance, class TIdx = uint16>
    struct TFixedCDTMesh2
    {
        using TIndex = TIdx;
        using THalfEdge = TMeshHalfEdge<TIdx>;
        using TFace = TMeshFace<TFaceData, TIdx>;
        using TVec = TVec2<TVecComp_>;
        using TVecComp = TVecComp_;
        static constexpr TVecComp DefaultThreshold = 1E-3;
        static constexpr size_t Capacity = NFaces;

        // Resets the mesh clearing all vertices, edges and faces.
        void Reset();

        ///////////////////////////////////////////////////////////////////////
        //
        // Vertices
        //
        ///////////////////////////////////////////////////////////////////////

        // Returns true if the index represents a valid vertex.
        bool IsValidVert(TIdx vertIndex) const;

        // Inserts an index into the mesh and returns the new index.
        // If there is already a vert in the mesh within the given threshold distance, that vertex's index is returned instead.
        TIdx InsertVertex(const TVec& pt, const TVecComp& threshold = DefaultThreshold);

        // Sets the position of a vertex.
        // Returns true if the index represents a valid vertex and the position is changed.
        bool SetVertex(TIdx vertIndex, const TVec& pt);

        // Returns a pointer to the vertex at the given index or nullptr if it doesn't exist.
        const TVec* GetVertexPtr(TIdx vertIndex) const;

        // Returns a reference to the vertex at the given index.
        // Use IsValidVert first to ensure that a valid vert exists.
        const TVec& GetVertex(TIdx vertIndex) const;

        // Returns the index of the closest vertex in the mesh to the given point and radius.
        TIdx FindClosestVertex(const TVec& pt, const TOptional<TVecComp>& radius) const;

        // Finds the indices for the two vertices that are closest to the given points.
        bool FindClosestVertices(
            const TVec& v0,
            const TVec& v1,
            TIdx& outV0,
            TIdx& outV1,
            const TVecComp& threshold = DefaultThreshold) const;

        // Returns true if any connected half-edges are locked.
        bool IsVertLocked(TIdx vertIndex) const;

        // Executes a callback for each vertex in range of a position.
        template <class T>
        void ForEachVertInRange(const TVec& pos, TVecComp radius, T& callback) const;

        // Executes a callback for each half-edge connected to a given vert.
        template <class T>
        void ForEachVertHalfEdge(TIdx vertIndex, T& callback, EHalfEdgeDirection direction = EHalfEdgeDirection::Both) const;

        ///////////////////////////////////////////////////////////////////////
        //
        // Half-Edges
        //
        ///////////////////////////////////////////////////////////////////////

        // Returns true if the index represents a valid half edge.
        bool IsValidHalfEdge(TIdx halfEdgeIndex) const;

        // Returns a pointer to the face at the given index or nullptr if it doesn't exist.
        const THalfEdge* GetHalfEdgePtr(TIdx halfEdgeIndex) const;

        // Returns a reference to the face at the given index.
        // Use IsValidFace first to ensure that a valid face exists.
        const THalfEdge& GetHalfEdge(TIdx halfEdgeIndex) const;

        // Inserts a new half edge with the given vertex indices.
        // If a half-edge with the given vertices already exists in the mesh, that half-edge's index is returned instead.
        TIdx InsertHalfEdge(TIdx vA, TIdx vB, TIdx f);

        // Returns the index of a half-edge that exists in the mesh with the given vertex positions.
        TIdx FindHalfEdge(
            const TVec& v0,
            const TVec& v1,
            TIdx& outV0,
            TIdx& outV1,
            const TVecComp& threshold = DefaultThreshold) const;

        // Returns the index of a half-edge that exists in the mesh with the given vertex indices.
        TIdx FindHalfEdge(TIdx v0, TIdx v1) const;

        // Gets the vertex positions of a given edge given the index of one of the half-edges.
        bool GetEdgeVerts(TIdx halfEdgeIndex, TVec& outVertA, TVec& outVertB) const;

        // Gets the center of an edge given the index of one of the half-edges.
        bool GetEdgeCenter(TIdx halfEdgeIndex, TVec& outCenter) const;

        // Gets the center and inner-normal of an edge given the index of one of the half-edges.
        bool GetEdgeCenterAndNormal(TIdx halfEdgeIndex, TVec& outCenter, TVec& outNormal) const;

        // Gets the magnitude of an edge given the index of one of the half-edges.
        auto GetEdgeLength(TIdx halfEdgeIndex) const;

        // Returns true if either half-edge of a given edge is locked.
        // TODO (jfarris): IF ONE IS LOCKED THEN THEY BOTH SHOULD BE! but they aren't always for some reason...
        bool IsEdgeLocked(TIdx halfEdgeIndex) const;

        ///////////////////////////////////////////////////////////////////////
        //
        // Edges
        //
        ///////////////////////////////////////////////////////////////////////

        // Returns the edge (half-edge pair) that exits in the mesh with the given vertex positions.
        TMeshEdge<TIdx> FindEdge(
            const TVec& v0,
            const TVec& v1,
            TIdx& outV0,
            TIdx& outV1,
            const TVecComp& threshold = DefaultThreshold) const;

        // Returns the edge (half-edge pair) that exits in the mesh with the given vertex indices.
        TMeshEdge<TIdx> FindEdge(TIdx v0, TIdx v1) const;

        ///////////////////////////////////////////////////////////////////////
        //
        // Faces
        //
        ///////////////////////////////////////////////////////////////////////

        // Returns true if the index represents a valid face.
        bool IsValidFace(TIdx faceIndex) const;

        // Returns a pointer to the face at the given index or nullptr if it doesn't exist.
        const TFace* GetFacePtr(TIdx faceIndex) const;

        // Returns a reference to the face at the given index.
        // Use IsValidFace first to ensure that a valid face exists.
        const TFace& GetFace(TIdx faceIndex) const;

        // Assumes that (a,b,c) is CCW
        TIdx InsertFace(TIdx v0, TIdx v1, TIdx v2, const TFaceData& data);

        // Inserts a new face into the mesh with the given vertex positions.
        // Assumes that (a,b,c) is CCW
        TIdx InsertFace(const TVec& a, const TVec& b, const TVec& c, const TFaceData& data);

        // Removes a face from the mesh.
        // Does not re-triangulate neighboring faces.
        bool RemoveFace(TIdx faceIndex);

        // Gets the vertex positions of the given face.
        bool GetFaceVerts(TIdx faceIndex, TVec& outA, TVec& outB, TVec& outC) const;

        // Gets the vertex positions of the given face.
        bool GetFaceVertPtrs(TIdx faceIndex, const TVec*& outA, const TVec*& outB, const TVec*& outC) const;

        // Returns the center of a face.
        bool GetFaceCenter(TIdx faceIndex, TVec& outCenter) const;

        // Returns the area of a face.
        bool GetFaceArea(TIdx faceIndex, TVecComp& outArea) const;

        // Returns the bounding box of a face.
        bool GetFaceBounds(TIdx faceIndex, TFixedBox<TVec>& outBounds) const;

        // Gets the index of the face containing the point or Index<TIdx>::None.
        TIdx FindFaceContainingPoint(const TVec& pos) const;

        // Returns whether a point is inside, outside or on the edge of a given face.
        PointInFaceResult<TIdx> IsPointInFace(TIdx f, const TVec& p) const;

        void SplitFace(TIdx faceIndex, TIdx vertIndex, TIdx& outFace0, TIdx& outFace1, TIdx& outFace2);

        void SplitEdge(TIdx edgeIndex, TIdx vertIndex);

        template <class TPredicate>
        void ForEachHalfEdgeInFace(TIdx faceIndex, const TPredicate& pred) const;

        template <class TPredicate>
        void ForEachHalfEdgeIndexInFace(TIdx faceIndex, const TPredicate& pred) const;

        template <class TPredicate>
        void ForEachHalfEdgeTwinInFace(TIdx faceIndex, const TPredicate& pred) const;

        template <class TPredicate>
        void ForEachNeighboringFaceIndex(TIdx faceIndex, const TPredicate& pred) const;

        template <class TPredicate>
        void ForEachNeighboringFace(TIdx faceIndex, const TPredicate& pred) const;

        ///////////////////////////////////////////////////////////////////////
        //
        // Operations
        //
        ///////////////////////////////////////////////////////////////////////

        // Recursively flips edges connected to the vertex until the delaunay condition is met.
        void FixDelaunayConditions(TIdx vi);

        void TracePortalEdges(const auto& corridor, auto& outChainRhs, auto& outChainLhs, bool trimDuplicates = true) const;

        void TracePortalEdgeVerts(const auto& corridor, auto& outChainRhs, auto& outChainLhs) const;

        void TriangulatePolygon(auto& chain, const TFaceData& faceData);

        TIdx CDT_InsertPoint(const TVec& v, bool fixDelaunayConditions = true);

        bool CDT_InsertEdge(const TLine<TVec>& line, bool fixDelaunayConditions = true);

        TFixedArray<TVec, NFaces*3> Vertices;
        TFixedArray<THalfEdge, NFaces*3> HalfEdges;
        TFixedArray<TFace, NFaces> Faces;
    };

    using DefaultFixedCDTMesh2 = TFixedCDTMesh2<8192, uint32, Distance, uint16>;
}

#include "Mesh2.inl"