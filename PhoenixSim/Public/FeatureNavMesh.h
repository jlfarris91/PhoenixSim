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

            NavMesh DynamicNavMesh;
        };

        struct PHOENIXSIM_API FeatureNavMeshScratchBlock
        {
            DECLARE_WORLD_BLOCK_DYNAMIC(FeatureNavMeshScratchBlock)

            mutable TMeshPath<NavMesh> MeshPath;
        };

        struct PHOENIXSIM_API PathResult
        {
            bool PathFound = false;
            NavMesh::TVert NextPoint;
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

            void OnPreUpdate(WorldRef world, const FeatureUpdateArgs& args) override;

            void OnHandleAction(WorldRef world, const FeatureActionArgs& action) override;

            void OnDebugRender(WorldConstRef world, const IDebugState& state, IDebugRenderer& renderer) override;

            // Inserts a new point into the dynamic nav mesh.
            static NavMesh::TIndex InsertPoint(WorldRef world, const NavMesh::TVert& pt);

            // Inserts a new edge into the dynamic nav mesh.
            static bool InsertEdge(WorldRef world, const NavMesh::TVert& start, const NavMesh::TVert& end);

            // Returns whether an agent with a given radius can path from start to end.
            static PathResult PathTo(
                WorldConstRef world,
                const NavMesh::TVert& start,
                const NavMesh::TVert& goal,
                NavMesh::TVertComp radius);

            // Returns whether an agent with a given radius can path from start to end.
            static PathResult CanPathTo(
                WorldConstRef world,
                const NavMesh::TVert& start,
                const NavMesh::TVert& goal,
                NavMesh::TVertComp radius);

        private:

            bool bDebugDrawVertices = true;
            bool bDebugDrawVertexIds = true;
            bool bDebugDrawHalfEdges = true;
            bool bDebugDrawHalfEdgeIds = true;
            bool bDebugDrawFaceIds = true;
        };
    }
}

