
#pragma once

#include "CTZ.h"
#include "Platform.h"
#include "Containers/FixedArray.h"
#include "FixedPoint/FixedVector.h"
#include "FixedPoint/FixedBox.h"

namespace Phoenix
{
    template <class T, uint32 N, class TVec = Vec2, class TIdx = uint16>
    struct TFixedBVH
    {
        using TBox = TFixedBox<TVec>;

        enum class ENodeType : uint8
        {
            None,
            Leaf,
            Parent
        };

        struct Node
        {
            struct Leaf
            {
                T Value;
            };

            struct Parent
            {
                TIdx Left;
                TIdx Right;
            };

            union
            {
                Leaf LeafData;      // Renamed to avoid conflict with struct name
                Parent ParentData;  // Renamed to avoid conflict with struct name
            };

            TBox Bounds;
            ENodeType Type = ENodeType::None;
        };

        TIdx Insert(const TBox& bounds, const T& value)
        {
            // uint64 mortonCode = bounds.GetCenter();
            // CommonPrefix()
            return Index<TIdx>::None;
        }

        template <class TList, class TBoundsFunc, class TValueFunc>
        TIdx InsertRange(
            const TList& list,
            TIdx begin,
            TIdx end,
            const TBoundsFunc& boundsFunc,
            const TValueFunc& valueFunc)
        {
            return Index<TIdx>::None;
            // if (begin == end)
            // {
            //     return AllocateLeafNode(boundsFunc(list, begin), valueFunc(list, begin));
            // }
            //
            // TIdx m = GetSplitPos(begin, end);
            // TIdx left = InsertRange(list, begin, m - 1);
            // TIdx right = InsertRange(list, m, end);
            // if (left == Index<TIdx>::None || right == Index<TIdx>::None)
            // {
            //     return Index<TIdx>::None;
            // }
            //
            // TBox bounds = TBox::Union(Nodes[left].Bounds, Nodes[right].Bounds);
            // return AllocateParentNode(bounds, left, right);
        }

        void Reset()
        {
            Nodes.Reset();
        }

        static uint64 CommonPrefix(uint64 a, uint64 b)
        {
            // Count leading zeroes of differing bits
            return CLZ(a ^ b);
        }

        TIdx GetSplitPos(TIdx begin, TIdx end)
        {
            return floor((begin + end) / 2);
        }

        TIdx AllocateLeafNode(const TBox& bounds, const T& value)
        {
            if (Nodes.IsFull())
            {
                return Index<TIdx>::None;
            }

            Node& node = Nodes.AddDefaulted_GetRef();
            node.Type = ENodeType::Leaf;
            node.Bounds = bounds;
            node.LeafData.Value = value;
            return Nodes.Num() - 1;
        }

        TIdx AllocateParentNode(const TBox& bounds, uint16 left, uint16 right)
        {
            if (Nodes.IsFull())
            {
                return Index<TIdx>::None;
            }

            Node& node = Nodes.AddDefaulted_GetRef();
            node.Type = ENodeType::Parent;
            node.Bounds = bounds;
            node.ParentData.Left = left;
            node.ParentData.Right = right;
            return Nodes.Num() - 1;
        }

        TFixedArray<Node, N> Nodes;
    };
}
