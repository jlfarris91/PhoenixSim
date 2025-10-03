
#pragma once

#include <iterator>

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

        constexpr bool Contains(const T& value)
        {
            for (size_t i = Start; i != End; i = MoveIndex(i, 1))
            {
                if (Data[i] == value)
                    return true;
            }
            return false;
        }

        constexpr void Enqueue(const T& value)
        {
            PHX_ASSERT(!IsFull());
            Data[End] = value;
            End = MoveIndex(End, 1);
        }

        constexpr void EnqueueUnique(const T& value)
        {
            if (Contains(value))
                return;
            Enqueue(value);
        }

        constexpr T Dequeue()
        {
            PHX_ASSERT(!IsEmpty());
            T value = Data[Start];
            Start = MoveIndex(Start, 1);
            return value;
        }

        constexpr void Reset()
        {
            Start = 0;
            End = 0;
        }

        static size_t MoveIndex(int32 i, int32 n)
        {
            i += n;
            if (i < 0) return N + i;
            return i % N;
        }

        T& operator[](size_t n)
        {
            return Data[MoveIndex(Start, n)];
        }

        const T& operator[](size_t n) const
        {
            return Data[MoveIndex(Start, n)];
        }

        struct Iter
        {
            using value_type = T;
            using element_type = T;
            using iterator_category = std::contiguous_iterator_tag;

            Iter(T* data, size_t index) : DataPtr(data), Index(index) {}

            T* operator->() const
            {
                return DataPtr + Index;
            }

            T& operator*() const
            {
                return *(DataPtr + Index);
            }

            T& operator[](int32 n) const
            {
                return *(DataPtr + MoveIndex(Index, n));
            }

            Iter& operator++()
            {
                Index = MoveIndex(Index, 1);
                return *this;
            }

            Iter operator++(int32 n) const
            {
                return { DataPtr, MoveIndex(Index, n) };
            }

            Iter& operator--()
            {
                Index = MoveIndex(Index, -1);
                return *this;
            }

            Iter operator--(int32 n) const
            {
                return { DataPtr, MoveIndex(Index, -n) };
            }

            Iter& operator+=(int32 n)
            {
                Index = MoveIndex(Index, n);
                return *this;
            }

            Iter& operator-=(int32 n)
            {
                Index = MoveIndex(Index, -n);
                return *this;
            }

            friend auto operator<=>(Iter, Iter) = default;

            friend long operator-(const Iter& a, const Iter& b)
            {
                if (a.Index > b.Index) return a.Index - b.Index;
                return a.Index + N - b.Index;
            }

            friend Iter operator+(Iter i, int32 n)
            {
                return { i.DataPtr, MoveIndex(i.Index, n) };
            }

            friend Iter operator-(Iter i, int32 n)
            {
                return { i.DataPtr, MoveIndex(i.Index, -n) };
            }

            friend Iter operator+(int32 n, Iter i)
            {
                return { i.DataPtr, MoveIndex(i.Index, n) };
            }

            bool operator==(const Iter& other) const
            {
                return DataPtr == other.DataPtr && Index == other.Index;
            }

        private:
            T* DataPtr;
            size_t Index;
        };

        Iter begin()
        {
            return Iter(&Data[0], Start);
        }

        Iter end()
        {
            return Iter(&Data[0], End);
        }

    private:

        T Data[N];
        size_t Start = 0, End = 0;
    };
}