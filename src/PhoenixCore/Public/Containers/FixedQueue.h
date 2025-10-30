
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
            return Num() == N - 1;
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

        static constexpr size_t MoveIndex(const size_t i, int64 n)
        {
            int64 r = static_cast<int64>(i) + n;
            if (r < 0) return N + r;
            return r % N;
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

            T& operator[](int64 n) const
            {
                return *(DataPtr + MoveIndex(Index, n));
            }

            Iter& operator++()
            {
                Index = MoveIndex(Index, 1);
                return *this;
            }

            Iter operator++(int32) const
            {
                auto prev = *this;
                ++*this;
                return prev;
            }

            Iter& operator--()
            {
                Index = MoveIndex(Index, -1);
                return *this;
            }

            Iter operator--(int32) const
            {
                auto prev = *this;
                --*this;
                return prev;
            }

            Iter& operator+=(int64 n)
            {
                Index = MoveIndex(Index, n);
                return *this;
            }

            Iter operator+(int64 n)
            {
                auto next = *this;
                next += n;
                return next;
            }

            Iter& operator-=(int64 n)
            {
                Index = MoveIndex(Index, -n);
                return *this;
            }

            Iter operator-(int64 n)
            {
                auto next = *this;
                next -= n;
                return next;
            }

            int64 operator-(const Iter& other) const
            {
                return static_cast<int64>(Index) - static_cast<int64>(other.Index);
            }

            bool operator==(const Iter& other) const
            {
                return DataPtr == other.DataPtr && Index == other.Index;
            }

            friend auto operator<=>(const Iter&, const Iter&) = default;

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

        struct ConstIter
        {
            using value_type = T;
            using element_type = T;
            using iterator_category = std::contiguous_iterator_tag;

            ConstIter(const T* data, size_t index) : DataPtr(data), Index(index) {}

            const T* operator->() const
            {
                return DataPtr + Index;
            }

            const T& operator*() const
            {
                return *(DataPtr + Index);
            }

            const T& operator[](int32 n) const
            {
                return *(DataPtr + MoveIndex(Index, n));
            }

            ConstIter& operator++()
            {
                Index = MoveIndex(Index, 1);
                return *this;
            }

            ConstIter operator++(int32 n) const
            {
                return { DataPtr, MoveIndex(Index, n) };
            }

            ConstIter& operator--()
            {
                Index = MoveIndex(Index, -1);
                return *this;
            }

            ConstIter operator--(int32 n) const
            {
                return { DataPtr, MoveIndex(Index, -n) };
            }

            ConstIter& operator+=(int32 n)
            {
                Index = MoveIndex(Index, n);
                return *this;
            }

            ConstIter& operator-=(int32 n)
            {
                Index = MoveIndex(Index, -n);
                return *this;
            }

            friend auto operator<=>(ConstIter, ConstIter) = default;

            friend long operator-(const ConstIter& a, const ConstIter& b)
            {
                if (a.Index > b.Index) return a.Index - b.Index;
                return a.Index + N - b.Index;
            }

            friend ConstIter operator+(ConstIter i, int32 n)
            {
                return { i.DataPtr, MoveIndex(i.Index, n) };
            }

            friend ConstIter operator-(ConstIter i, int32 n)
            {
                return { i.DataPtr, MoveIndex(i.Index, -n) };
            }

            friend ConstIter operator+(int32 n, ConstIter i)
            {
                return { i.DataPtr, MoveIndex(i.Index, n) };
            }

            bool operator==(const ConstIter& other) const
            {
                return DataPtr == other.DataPtr && Index == other.Index;
            }

        private:
            const T* DataPtr;
            size_t Index;
        };

        ConstIter begin() const
        {
            return ConstIter(&Data[0], Start);
        }

        ConstIter end() const
        {
            return ConstIter(&Data[0], End);
        }

    private:

        T Data[N];
        size_t Start = 0, End = 0;
    };

    static_assert(TFixedQueue<int, 32>::MoveIndex(0, 32) == 0);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, 31) == 31);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, 30) == 30);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, 3) == 3);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, 2) == 2);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, 1) == 1);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, 0) == 0);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, -1) == 31);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, -2) == 30);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, -3) == 29);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, -32) == 0);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, -31) == 1);
    static_assert(TFixedQueue<int, 32>::MoveIndex(0, -30) == 2);
}