
#pragma once

#include "PhoenixCore.h"
#include "Platform.h"

namespace Phoenix
{
    template <size_t N>
    struct TFixedArena
    {
        static constexpr size_t Capacity = N;

        struct Handle { uint16 Id; };

        static constexpr Handle InvalidHandle = { 0 };

        struct Arena
        {
            uint16 Id = 0;
            Arena* Next;
        };

        constexpr bool IsEmpty() const
        {
            return TailArena == nullptr;
        }

        constexpr bool IsFull() const
        {
            return Size == N;
        }

        constexpr size_t GetSize() const
        {
            return Size;
        }

        void Reset()
        {
            TailArena = nullptr;
            ArenaIdCounter = 0;
            Size = 0;
        }

        Handle Allocate(size_t size)
        {
            Arena* arena = AllocateArena(size);
            if (arena == nullptr)
            {
                return InvalidHandle;
            }
            return arena->Id;
        }

        template <class T, class ...TArgs>
        Handle Emplace(const TArgs&... args)
        {
            Arena* arena = AllocateArena(sizeof(T));
            if (!arena)
            {
                return InvalidHandle;
            }

            void* data = GetArenaData(arena);
            T* typedData = static_cast<T*>(data);
            new (typedData) T(args...);

            return arena->Id;
        }

        template <class T = void>
        T* Get(const Handle& handle)
        {
            void* data = GetArenaData(handle);
            return static_cast<T*>(data);
        }

        template <class T = void>
        const T* Get(const Handle& handle) const
        {
            const void* data = GetArenaData(handle);
            return static_cast<const T*>(data);
        }

        void* operator[](const Handle& handle)
        {
            return GetArenaData(handle);
        }

        const void* operator[](const Handle& handle) const
        {
            return GetArenaData(handle);
        }

    private:

        Arena* GetFirstArena()
        {
            if (IsEmpty())
            {
                return nullptr;
            }
            return static_cast<Arena*>(&Data[0]);
        }

        const Arena* GetFirstArena() const
        {
            if (IsEmpty())
            {
                return nullptr;
            }
            return static_cast<const Arena*>(&Data[0]);
        }

        Arena* GetArena(const Handle& handle)
        {
            Arena* arena = GetFirstArena();
            while (arena && arena->Id != handle.Id)
            {
                arena = arena->Next;
            }

            return arena;
        }

        const Arena* GetArena(const Handle& handle) const
        {
            Arena* arena = GetFirstArena();
            while (arena && arena->Id != handle.Id)
            {
                arena = arena->Next;
            }

            return arena;
        }

        void* GetArenaData(const Handle& handle)
        {
            Arena* arena = GetArena(handle);
            if (!arena)
            {
                return nullptr;
            }
            return GetArenaData(arena);
        }

        const void* GetArenaData(const Handle& handle) const
        {
            const Arena* arena = GetArena(handle);
            if (!arena)
            {
                return nullptr;
            }
            return GetArenaData(arena);
        }

        static void* GetArenaData(Arena* arena)
        {
            return static_cast<void*>(arena) + sizeof(Arena);
        }

        static void* GetArenaData(const Arena* arena)
        {
            return static_cast<const void*>(arena) + sizeof(Arena);
        }

        Arena* AllocateArena(size_t size)
        {
            if (Size + size >= N)
            {
                return nullptr;
            }

            void* block = Data + Size;
            Arena* arena = static_cast<Arena*>(block);
            arena->Id = ++ArenaIdCounter;
            arena->Next = nullptr;

            void* data = GetArenaData(arena);
            memset(data, 0, size);

            if (TailArena)
            {
                TailArena->Next = arena;
            }

            TailArena = arena;

            Size += size;

            return arena;
        }

        uint8 Data[N] = {};
        size_t Size = 0;
        uint16 ArenaIdCounter = 0;
        Arena* TailArena = nullptr;
    };
}
