#pragma once

#include "Features.h"
#include "Mesh2.h"
#include "MeshPath.h"


namespace Phoenix
{
    namespace Pathfinding
    {

#ifndef PATH_MESH_TYPE
        using NavMesh = TFixedCDTMesh2<8192, uint32, Distance, uint16>;
#endif

        struct PHOENIXSIM_API FeatureNavMeshStaticBlock
        {
            DECLARE_WORLD_BLOCK_DYNAMIC(FeatureNavMeshStaticBlock)

            NavMesh StaticNavMesh;
        };

        struct PHOENIXSIM_API FeatureNavMeshDynamicBlock
        {
            DECLARE_WORLD_BLOCK_DYNAMIC(FeatureNavMeshDynamicBlock)

            Vec2 MapSize;
            NavMesh DynamicNavMesh;
            TFixedArray<Line2, NavMesh::Capacity> DynamicEdges;
            TFixedArray<Vec2, NavMesh::Capacity> DynamicPoints;
            bool bDirty = true;
        };

        struct PHOENIXSIM_API FeatureNavMeshScratchBlock
        {
            DECLARE_WORLD_BLOCK_DYNAMIC(FeatureNavMeshScratchBlock)

            mutable TMeshPath<NavMesh> MeshPath;
        };

        struct PHOENIXSIM_API PathResult
        {
            bool PathFound = false;
            NavMesh::TVec NextPoint;
        };

        class PHOENIXSIM_API FeatureNavMesh : public IFeature
        {
            FEATURE_BEGIN(FeatureNavMesh)
                FEATURE_BLOCK(FeatureNavMeshStaticBlock)
                FEATURE_BLOCK(FeatureNavMeshDynamicBlock)
                FEATURE_BLOCK(FeatureNavMeshScratchBlock)
                FEATURE_CHANNEL(WorldChannels::PreUpdate)
                FEATURE_CHANNEL(WorldChannels::HandleAction)
                FEATURE_CHANNEL(WorldChannels::DebugRender)
            FEATURE_END()

        public:

            FeatureNavMesh();

            void RebuildNavMesh(WorldRef world);
            
            void OnPreUpdate(WorldRef world, const FeatureUpdateArgs& args) override;

            void OnHandleAction(WorldRef world, const FeatureActionArgs& action) override;

            void OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer) override;

            // Inserts a new point into the dynamic nav mesh.
            static NavMesh::TIndex InsertPoint(WorldRef world, const NavMesh::TVec& pt);

            // Inserts a new edge into the dynamic nav mesh.
            static bool InsertEdge(WorldRef world, const NavMesh::TVec& start, const NavMesh::TVec& end);

            // Returns whether an agent with a given radius can path from start to end.
            static PathResult PathTo(
                WorldConstRef world,
                const NavMesh::TVec& start,
                const NavMesh::TVec& goal,
                NavMesh::TVecComp radius);

            // Returns whether an agent with a given radius can path from start to end.
            static PathResult CanPathTo(
                WorldConstRef world,
                const NavMesh::TVec& start,
                const NavMesh::TVec& goal,
                NavMesh::TVecComp radius);

        private:

            bool bDebugDrawVertices = false;
            bool bDebugDrawVertexIds = false;
            bool bDebugDrawHalfEdges = false;
            bool bDebugDrawHalfEdgeIds = false;
            bool bDebugDrawFaceIds = false;
        };
    }
}

