
#pragma once

#include "Platform.h"

namespace Phoenix
{
    template <class T>
    struct TOptional
    {
        TOptional() = default;
        TOptional(const T& value) : Value(value), bHasValue(true) {}
        TOptional(T&& value) noexcept : Value(std::move(value)), bHasValue(true) {}
        TOptional(const TOptional& other) : Value(other.Value), bHasValue(other.bHasValue) {}
        TOptional(TOptional&& other) : Value(std::move(other.Value)), bHasValue(std::move(other.bHasValue)) {}

        bool IsSet() const
        {
            return bHasValue;
        }

        T& Get()
        {
            PHX_ASSERT(bHasValue);
            return Value;
        }

        const T& Get() const
        {
            PHX_ASSERT(bHasValue);
            return Value;
        }

        T* GetPtr()
        {
            PHX_ASSERT(bHasValue);
            return &Value;
        }

        const T* GetPtr() const
        {
            PHX_ASSERT(bHasValue);
            return &Value;
        }

        T* GetPtrOrNull() const
        {
            return bHasValue ? &Value : nullptr;
        }

        T GetValue(const T& defaultValue) const
        {
            return bHasValue ? Value : defaultValue;
        }

        void Reset()
        {
            Value.~T();
            bHasValue = false;
        }

        bool operator==(const T& value) const
        {
            return bHasValue && Value == value;
        }

        bool operator!=(const T& value) const
        {
            return !operator==(value);
        }

        bool operator==(const TOptional& other) const
        {
            return bHasValue == other.bHasValue && Value == other.Value;
        }

        bool operator!=(const TOptional& other) const
        {
            return !operator==(other);
        }

        T& operator*()
        {
            return Get();
        }

        const T& operator*() const
        {
            return Get();
        }

        T* operator->()
        {
            return GetPtr();
        }

        const T* operator->() const
        {
            return GetPtr();
        }

        TOptional& operator=(const T& value)
        {
            Value = value;
            bHasValue = true;
            return *this;
        }

        TOptional& operator=(T&& value)
        {
            Value = std::move(value);
            bHasValue = true;
            return *this;
        }

        TOptional& operator=(const TOptional& other)
        {
            Value = other.Value;
            bHasValue = other.bHasValue;
            return *this;
        }

        TOptional& operator=(TOptional&& other) noexcept
        {
            Value = std::move(other.Value);
            bHasValue = std::move(other.bHasValue);
            return *this;
        }

    private:

        T Value = {};
        bool bHasValue = false;
    };
}
