
#pragma once

#include "Platform.h"

namespace Phoenix
{
    template <size_t N, size_t BlockSize>
    struct TFixedBlockAllocator
    {
        static constexpr auto Capacity = N;

        struct Handle
        {
            uint32 Id = Index<uint32>::None;
        };

        struct Block
        {
            uint32 Id = 0;
            uint32 UserData = 0;
            uint8 Data[BlockSize] = {};
        };

        constexpr uint32 GetNumBlocks() const
        {
            return (uint32)Blocks.Num();
        }

        constexpr uint32 GetNumOccupiedBlocks() const
        {
            return NumOccupiedBlocks;
        }

        constexpr bool IsEmpty() const
        {
            return NumOccupiedBlocks == 0;
        }

        constexpr bool IsFull() const
        {
            return NumOccupiedBlocks == Capacity;
        }

        constexpr bool IsValid(Handle handle) const
        {
            if (handle.Id == Index<uint32>::None)
            {
                return false;
            }

            uint32 index = GetBlockIndex(handle);
            return Blocks.IsValidIndex(index) && Blocks[index].Id == handle.Id;
        }

        Handle Allocate(uint32 userData)
        {
            if (IsFull())
            {
                return Handle();
            }

            uint32 index = AllocateBlock(userData);
            if (index == Index<uint32>::None)
            {
                return Handle();
            }

            Block& block = Blocks[index];
            memset(block.Data, 0, BlockSize);

            return { block.Id };
        }

        template <class T, class ...TArgs>
        Handle Allocate(uint32 userData, TArgs&& ...args)
        {
            if (IsFull())
            {
                return Handle();
            }

            uint32 index = AllocateBlock(userData);
            if (index == Index<uint32>::None)
            {
                return Handle();
            }

            Block& block = Blocks[index];
            new (block.Data) T(std::forward<TArgs>(args)...);

            return { block.Id };
        }

        bool Deallocate(const Handle& handle)
        {
            if (!IsValid(handle))
            {
                return false;
            }

            uint32 index = GetBlockIndex(handle);
            Block& block = Blocks[index];
            IndexMap.Remove(block.Id);
            block.Id = Index<uint32>::None;

            --NumOccupiedBlocks;

            return true;
        }

        void* GetPtr(const Handle& handle)
        {
            uint32 index = GetBlockIndex(handle);
            if (!Blocks.IsValidIndex(index))
            {
                return nullptr;
            }

            return Blocks[index].Data;
        }

        const void* GetPtr(const Handle& handle) const
        {
            uint32 index = GetBlockIndex(handle);
            if (!Blocks.IsValidIndex(index))
            {
                return nullptr;
            }

            return Blocks[index].Data;
        }

        template <class T>
        T* GetPtr(Handle handle)
        {
            return static_cast<T*>(GetPtr(handle));
        }

        template <class T>
        const T* GetPtr(Handle handle) const
        {
            return static_cast<const T*>(GetPtr(handle));
        }

        // Re-organize blocks so that all occupied blocks are at the front.
        void Compact()
        {
            if (Blocks.IsEmpty())
                return;

            //               i ->        <- j
            // Blocks:      [3][-][-][1][-][2]  => [3][2][1][-][-][-]
            // IndexMap:    {1,3}, {2,5}, {3,0} => {1,2}, {2,1}, {3,0}
            uint32 i = 0;
            uint32 j = (uint32)Blocks.Num() - 1;
            while (i < j)
            {
                // When moving forward, skip blocks that are already occupied
                if (IsBlockOccupied(i))
                {
                    ++i;
                    continue;
                }

                // Walk backwards from the end until we find the first occupied block.
                while (j > i && !IsBlockOccupied(j))
                {
                    --j;
                }

                // There are no more blocks to move.
                if (i == j)
                {
                    break;
                }

                // Move block at index j to index i
                {                    
                    // Block handle j now points to block at index i
                    IndexMap[Blocks[j].Id] = i;

                    // Copy the block data to the new address
                    memcpy(&Blocks[i], &Blocks[j], sizeof(Block));

                    // Invalidate block at index j
                    Blocks[j].Id = Index<uint32>::None;
                }

                ++i;
            }

            // Set the size of the array to the number of occupied blocks at the front.
            Blocks.SetSize(NumOccupiedBlocks);
        }

        struct ConstIter
        {
            ConstIter(const TFixedBlockAllocator* owner, uint32 index) : Index(index), Owner(owner)
            {
                Index = Owner->FindNextOccupiedBlockIndex(Index);
            }

            Handle operator*() const
            {
                if (!Owner->Blocks.IsValidIndex(Index))
                    return Handle();
                return { Owner->Blocks[Index].Id };
            }

            ConstIter& operator++()
            {
                Index = Owner->FindNextOccupiedBlockIndex(Index + 1);
                return *this;
            }

            bool operator==(const ConstIter& other) const = default;
            bool operator!=(const ConstIter& other) const = default;

            uint32 Index;
            const TFixedBlockAllocator* Owner;
        };

        ConstIter begin() const { return ConstIter(this, FindNextOccupiedBlockIndex(0)); }
        ConstIter end() const { return ConstIter(this, (uint32)Blocks.Num()); }

    private:

        // Gets the index of a block given a block handle.
        uint32 GetBlockIndex(const Handle& handle) const
        {
            if (const uint32* indexPtr = IndexMap.GetPtr(handle.Id))
            {
                return *indexPtr;
            }

            return handle.Id;
        }

        // Returns true if the block at the index is occupied.
        bool IsBlockOccupied(uint32 index) const
        {
            return Blocks.IsValidIndex(index) && Blocks[index].Id != Index<uint32>::None;
        }

        uint32 FindNextOccupiedBlockIndex(uint32 index) const
        {
            while (index < Blocks.Num() && !IsBlockOccupied(index))
            {
                ++index;
            }
            return index;
        }

        // Returns the index of the first unoccupied block or -1 if there are no free blocks.
        uint32 FindFreeBlock() const
        {
            for (uint32 i = 0; i < Blocks.Num(); ++i)
            {
                if (!IsBlockOccupied(i))
                    return i;
            }
            return Index<uint32>::None;
        }

        // Returns the index of a block or -1 if the array is full.
        uint32 AllocateBlock(uint32 userData)
        {
            uint32 index = FindFreeBlock();

            // No free blocks, try to add a new one.
            if (index == Index<uint32>::None)
            {
                if (Blocks.IsFull())
                {
                    return Index<uint32>::None;
                }

                index = (uint32)Blocks.Num();

                Block& block = Blocks.AddDefaulted_GetRef();
                block.Id = ++BlockIdGen;
            }

            Block& block = Blocks[index];
            block.UserData = userData;
            IndexMap.Insert(block.Id, index);

            ++NumOccupiedBlocks;
            
            return index;
        }

        TFixedArray<Block, Capacity> Blocks;
        TFixedMap<uint32, uint32, Capacity> IndexMap;
        uint32 BlockIdGen = 0;
        uint32 NumOccupiedBlocks = 0;
    };
}
