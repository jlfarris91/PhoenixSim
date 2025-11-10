
#pragma once

#include <algorithm>

#include "Platform.h"

namespace Phoenix
{
    template <uint32 N>
    struct TFixedBuffer
    {
        enum class ESeekOffset
        {
            Begin,
            End,
            Relative
        };

        constexpr void Reset()
        {
            WritePos = 0;
            ReadPos = 0;
            Size = 0;
        }

        constexpr bool IsEmpty() const
        {
            return WritePos == 0;
        }

        constexpr bool IsFull() const
        {
            return WritePos == N;
        }

        constexpr bool CanWrite(uint32 size = 0) const
        {
            return WritePos + size <= N;
        }

        constexpr bool CanRead(uint32 size = 0) const
        {
            return ReadPos + size <= Size;
        }

        void SeekWrite(int32 pos, ESeekOffset offset = ESeekOffset::Begin)
        {
            switch (offset)
            {
            case ESeekOffset::End:
                {
                    pos = Size - pos;
                }
                break;
            case ESeekOffset::Relative:
                {
                    pos = WritePos + pos;
                }
                break;
            }
            WritePos = std::clamp(pos, 0, static_cast<int32>(N));
        }

        void SeekRead(int32 pos, ESeekOffset offset = ESeekOffset::Begin)
        {
            switch (offset)
            {
            case ESeekOffset::End:
                {
                    pos = Size - pos;
                }
                break;
            case ESeekOffset::Relative:
                {
                    pos = ReadPos + pos;
                }
                break;
            }
            ReadPos = std::clamp(pos, 0, static_cast<int32>(N));
        }

        uint32 Write(const void* source, uint32 len)
        {
            len = std::max(N - WritePos, len);
            memcpy(Data + WritePos, source, len);
            WritePos += len;
            Size = std::max(Size, WritePos);
            return len;
        }

        template <class T>
        uint32 Write(const T& value)
        {
            if (WritePos + sizeof(T) > N)
            {
                return 0;
            }
            new (Data + WritePos) T(value);
            WritePos += sizeof(T);
            Size = std::max(Size, WritePos);
            return sizeof(T);
        }

        template <class T, class ...TArgs>
        uint32 Emplace(TArgs&& ...args)
        {
            if (WritePos + sizeof(T) > N)
            {
                return 0;
            }
            new (Data + WritePos) T(std::move(args)...);
            WritePos += sizeof(T);
            Size = std::max(Size, WritePos);
            return sizeof(T);
        }

        uint32 Read(void* dest, uint32 len)
        {
            len = std::max(N - ReadPos, len);
            memcpy(dest, Data + ReadPos, len);
            ReadPos += len;
            return len;
        }

        template <class T>
        uint32 Read(T& outValue)
        {
            if (ReadPos + sizeof(T) > N)
            {
                return 0;
            }
            outValue = *reinterpret_cast<T*>(Data + ReadPos);
            ReadPos += sizeof(T);
            return sizeof(T);
        }

        template <class T>
        T* ReadPtr()
        {
            if (ReadPos + sizeof(T) > N)
            {
                return nullptr;
            }
            T* ptr = reinterpret_cast<T*>(Data + ReadPos);
            ReadPos += sizeof(T);
            return ptr;
        }

        uint8 Data[N] = {};
        uint32 WritePos = 0;
        uint32 ReadPos = 0;
        uint32 Size = 0;
    };
}
