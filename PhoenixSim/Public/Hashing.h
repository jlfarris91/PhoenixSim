
#pragma once

#include <cstdint>
#include "DLLExport.h"

namespace Phoenix
{
    typedef uint32_t hash_t;

    struct PHOENIXSIM_API Hashing
    {
        static constexpr hash_t FN1VA32(const char* chars, size_t length)
        {
            const hash_t prime = 0x1000193;
            hash_t hash = 0x811c9dc5;

            for (size_t i = 0; i < length; ++i)
            {
                uint8_t value = chars[i];
                hash = hash ^ value;
                hash *= prime;
            }

            return hash;
        }
    };
}
