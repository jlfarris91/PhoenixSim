
#pragma once

#include <stdlib.h>

#include "Hashing.h"

namespace Phoenix
{
    struct FName
    {
        static const FName None;
        static const FName Empty;

        constexpr FName() = default;
        constexpr FName(hash_t hash) : Hash(hash) {}

        constexpr FName(const char* chars, size_t len)
            : Hash(Hashing::FN1VA32(chars, len))
        {
#if DEBUG
            for (size_t i = 0; i < len && i < _countof(Debug); ++i)
                Debug[i] = *(chars + i);
#endif
        }

        operator hash_t() const;
        bool operator==(const FName& other) const;
        bool operator!=(const FName& other) const;

    private:
        hash_t Hash = 0;

#if DEBUG
    public:
        char Debug[64] = {};
#endif
    };

    constexpr FName operator ""_n(const char* chars, size_t len)
    {
        return FName(chars, len);
    }
}
