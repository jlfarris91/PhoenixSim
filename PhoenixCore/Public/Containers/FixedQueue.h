
#pragma once

namespace Phoenix
{
    template <class T, size_t N>
    struct TFixedQueue
    {
        using ValueT = T;
        static constexpr size_t Capacity = N;

        constexpr bool IsEmpty() const
        {
            return Start == End;
        }

        constexpr bool IsFull() const
        {
            return Num() == N;
        }

        constexpr size_t Num() const
        {
            if (Start > End)
                return End + N - Start;
            return End - Start;
        }

        constexpr void Enqueue(const T& value)
        {
            PHX_ASSERT(!IsFull());
            Items[End] = value;
            if (End++ == N)
                End = 0;
        }

        constexpr T Dequeue()
        {
            PHX_ASSERT(!IsEmpty());
            T value = Items[Start];
            if (Start++ == N)
                Start = 0;
            return value;
        }

        constexpr void Reset()
        {
            Start = 0;
            End = 0;
        }

        T Items[N];
        size_t Start = 0, End = 0;
    };
}