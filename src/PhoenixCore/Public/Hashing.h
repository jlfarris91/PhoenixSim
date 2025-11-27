
#pragma once

#include "Platform.h"

namespace Phoenix
{
    typedef uint32 hash32_t;
    typedef uint64 hash64_t;

    struct PHOENIXCORE_API Hashing
    {
        //
        // 32-bit
        //

        static constexpr hash32_t prime32 = 0x1000193;
        static constexpr hash32_t basis32 = 0x811c9dc5;

        static constexpr hash32_t FNV1A32(const char* data, size_t length, hash32_t basis = basis32)
        {
            hash32_t hash = basis;

            for (size_t i = 0; i < length; ++i)
            {
                hash = hash ^ data[i];
                hash *= prime32;
            }

            return hash;
        }

        static constexpr hash32_t FNV1A32(const void* data, size_t length, hash32_t basis = basis32)
        {
            hash32_t hash = basis;

            auto chars = static_cast<const char*>(data);
            for (size_t i = 0; i < length; ++i)
            {
                hash = hash ^ chars[i];
                hash *= prime32;
            }

            return hash;
        }

        template <class T>
        static constexpr hash32_t FNV1A32(const T& obj, hash32_t basis = basis32)
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

        //
        // 64-bit
        //

        static constexpr hash64_t prime64 = 0x100000001b3;
        static constexpr hash64_t basis64 = 0xcbf29ce484222325;
        
        static constexpr hash64_t FNV1A64(const char* data, size_t length, hash64_t basis = basis64)
        {
            hash64_t hash = basis;

            for (size_t i = 0; i < length; ++i)
            {
                hash = hash ^ data[i];
                hash *= prime64;
            }

            return hash;
        }

        static constexpr hash64_t FNV1A64(const void* data, size_t length, hash64_t basis = basis64)
        {
            hash64_t hash = basis;

            auto chars = static_cast<const char*>(data);
            for (size_t i = 0; i < length; ++i)
            {
                hash = hash ^ chars[i];
                hash *= prime64;
            }

            return hash;
        }

        template <class T>
        static constexpr hash64_t FNV1A64(const T& obj, hash64_t basis = basis64)
        {
            return FNV1A64(&obj, sizeof(T), basis);
        }

        static constexpr hash64_t FN1VA64Combine(hash64_t basis)
        {
            return basis;
        }

        template <class TArg0, class ...TArgs>
        static constexpr hash64_t FN1VA64Combine(hash64_t basis, TArg0&& arg0, TArgs&&... args)
        {
            auto h = FNV1A64(arg0, basis);
            h = FN1VA64Combine(h, args...);
            return h;
        }
    };
}
