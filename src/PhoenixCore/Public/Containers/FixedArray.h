#pragma once

#include <cassert>
#include <iterator>

namespace Phoenix
{
    template <class T, size_t N>
    class TFixedArray
    {
    public:
        using ItemT = T;
        static constexpr size_t Capacity = N;

        T& operator[](size_t index)
        {
            return *(Data + index);
        }

        const T& operator[](size_t index) const
        {
            return *(Data + index);
        }

        size_t Num() const
        {
            return Size;
        }

        size_t GetTotalSize() const
        {
            return Size * sizeof(T);
        }

        bool IsValidIndex(size_t index) const
        {
            return index < Size;
        }

        bool IsEmpty() const
        {
            return Size == 0;
        }

        bool IsFull() const
        {
            return Size == Capacity;
        }

        T& PushBack(const T& value)
        {
            assert(Size < Capacity);
            Data[Size++] = value;
            return Data[Size-1];
        }

        void Add(const T& value)
        {
            (void)PushBack(value);
        }

        T& Add_GetRef(const T& value)
        {
            return PushBack(value);
        }

        void AddDefaulted()
        {
            (void)PushBack({});
        }

        T& AddDefaulted_GetRef()
        {
            return PushBack({});
        }

        template <class ...TArgs>
        T& EmplaceBack_GetRef(TArgs... args)
        {
            assert(Size < Capacity);
            Data[Size++] = { args... };
            return Data[Size-1];
        }
        
        template <class ...TArgs>
        void EmplaceBack(TArgs... args)
        {
            (void)EmplaceBack_GetRef(args...);
        }

        void PopBack()
        {
            assert(Size > 0);
            Data[Size--].~T();
        }

        T PopBackAndReturn()
        {
            assert(Size > 0);
            T temp = Data[Size - 1];
            Data[Size--].~T();
            return temp;
        }

        T& Front()
        {
            assert(Size > 0);
            return Data[0];
        }

        const T& Front() const
        {
            assert(Size > 0);
            return Data[0];
        }

        T& Back()
        {
            assert(Size > 0);
            return Data[Size - 1];
        }

        const T& Back() const
        {
            assert(Size > 0);
            return Data[Size - 1];
        }

        void Reset()
        {
            Size = 0;
        }

        void SetNum(size_t newSize, const T& value = {})
        {
            while (Size < newSize)
                Add_GetRef(value);
            while (Size > newSize)
                PopBack();
        }

        void SetSize(size_t newSize)
        {
            Size = newSize >= Capacity ? Capacity : newSize;
        }

        void Fill(const T& value = {})
        {
            SetNum(Capacity, value);
        }

        int32 IndexOf(const T& value)
        {
            for (size_t i = 0; i < Size; ++i)
            {
                if (Data[i] == value)
                    return (int32)i;
            }
            return INDEX_NONE;
        }

        bool Contains(const T& value)
        {
            return IndexOf(value) != INDEX_NONE;
        }

        void RemoveAt(size_t index)
        {
            for (size_t i = index; i < Size - 1; ++i)
            {
                Data[i] = Data[i + 1];
            }
            --Size;
        }

        struct Iter
        {
            using value_type = T;
            using element_type = T;
            using iterator_category = std::contiguous_iterator_tag;

            Iter(T* data = nullptr) : DataPtr(data) {}

            T* operator->() const
            {
                return DataPtr;
            }

            T& operator*() const
            {
                return *DataPtr;
            }

            T& operator[](int n) const
            {
                return *(DataPtr + n);
            }

            Iter& operator++()
            {
                ++DataPtr;
                return *this;
            }

            Iter operator++(int n) const
            {
                return { DataPtr + n };
            }

            Iter& operator--()
            {
                --DataPtr;
                return *this;
            }

            Iter operator--(int n) const
            {
                return { DataPtr - n };
            }

            Iter& operator+=(int n)
            {
                DataPtr += n;
                return *this;
            }

            Iter& operator-=(int n)
            {
                DataPtr -= n;
                return *this;
            }

            friend auto operator<=>(Iter, Iter) = default;

            friend auto operator-(const Iter& a, const Iter& b)
            {
                return a.DataPtr - b.DataPtr;
            }

            friend Iter operator+(Iter i, int n)
            {
                return { i.DataPtr + n };
            }

            friend Iter operator-(Iter i, int n)
            {
                return { i.DataPtr - n };
            }

            friend Iter operator+(int n, Iter i)
            {
                return { i.DataPtr + n };
            }

            bool operator==(const Iter& other) const
            {
                return DataPtr == other.DataPtr;
            }

        private:
            T* DataPtr;
        };

        Iter begin()
        {
            return Iter(Data);
        }

        Iter end()
        {
            return Iter(Data + Size);
        }

        struct ConstIter
        {
            using value_type = T;
            using element_type = T;
            using iterator_category = std::contiguous_iterator_tag;

            ConstIter(const T* data = nullptr) : DataPtr(data) {}

            const T* operator->() const
            {
                return DataPtr;
            }

            const T& operator*() const
            {
                return *DataPtr;
            }

            const T& operator[](int n) const
            {
                return *(DataPtr + n);
            }

            ConstIter& operator++()
            {
                ++DataPtr;
                return *this;
            }

            ConstIter operator++(int n) const
            {
                return { DataPtr + n };
            }

            ConstIter& operator--()
            {
                --DataPtr;
                return *this;
            }

            ConstIter operator--(int n) const
            {
                return { DataPtr - n };
            }

            ConstIter& operator+=(int n)
            {
                DataPtr += n;
                return *this;
            }

            ConstIter& operator-=(int n)
            {
                DataPtr -= n;
                return *this;
            }

            friend auto operator<=>(ConstIter, ConstIter) = default;

            friend auto operator-(const ConstIter& a, const ConstIter& b)
            {
                return a.DataPtr - b.DataPtr;
            }

            friend ConstIter operator+(ConstIter i, int n)
            {
                return { i.DataPtr + n };
            }

            friend ConstIter operator-(ConstIter i, int n)
            {
                return { i.DataPtr - n };
            }

            friend ConstIter operator+(int n, ConstIter i)
            {
                return { i.DataPtr + n };
            }

            bool operator==(const ConstIter& other) const
            {
                return DataPtr == other.DataPtr;
            }

        private:
            const T* DataPtr;
        };

        ConstIter begin() const
        {
            return ConstIter(Data);
        }

        ConstIter end() const
        {
            return ConstIter(Data + Size);
        }

    private:
        T Data[N];
        size_t Size = 0;
    };

    static_assert(std::contiguous_iterator<TFixedArray<int, 1>::Iter>);
    static_assert(std::contiguous_iterator<TFixedArray<int, 1>::ConstIter>);
}
