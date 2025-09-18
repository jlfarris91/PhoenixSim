
#pragma once

#include <cstdint>
#include "DLLExport.h"

namespace Phoenix
{
    typedef uint32_t hash_t;

    struct PHOENIXSIM_API Hashing
    {
        static constexpr hash_t FN1VA32(const char* data, size_t length)
        {
            const hash_t prime = 0x1000193;
            hash_t hash = 0x811c9dc5;

            for (size_t i = 0; i < length; ++i)
            {
                hash = hash ^ data[i];
                hash *= prime;
            }

            return hash;
        }

        static hash_t FN1VA32(const void* data, size_t length)
        {
            const hash_t prime = 0x1000193;
            hash_t hash = 0x811c9dc5;

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
        static constexpr hash_t FN1VA32(const T& obj)
        {
            return FN1VA32(&obj, sizeof(T));
        }
    };
}
