
#pragma once

#include "Platform.h"

namespace Phoenix
{
    typedef uint32 hash32_t;

    struct PHOENIXCORE_API Hashing
    {
        static constexpr hash32_t FNV1A32(const char* data, size_t length, hash32_t basis = 0x811c9dc5)
        {
            const hash32_t prime = 0x1000193;
            hash32_t hash = basis;

            for (size_t i = 0; i < length; ++i)
            {
                hash = hash ^ data[i];
                hash *= prime;
            }

            return hash;
        }

        static hash32_t FNV1A32(const void* data, size_t length, hash32_t basis = 0x811c9dc5)
        {
            const hash32_t prime = 0x1000193;
            hash32_t hash = basis;

            // Cannot be constexpr because of this cast
            auto chars = static_cast<const char*>(data);
            for (size_t i = 0; i < length; ++i)
            {
                hash = hash ^ chars[i];
                hash *= prime;
            }

            return hash;
        }

        template <class T>
        static constexpr hash32_t FNV1A32(const T& obj, hash32_t basis = 0x811c9dc5)
        {
            return FNV1A32(&obj, sizeof(T), basis);
        }

        static constexpr hash32_t FN1VA32Combine(hash32_t basis)
        {
            return basis;
        }

        template <class TArg0, class ...TArgs>
        static constexpr hash32_t FN1VA32Combine(hash32_t basis, TArg0&& arg0, TArgs&&... args)
        {
            auto h = FNV1A32(arg0, basis);
            h = FN1VA32Combine(h, args...);
            return h;
        }
    };
}
