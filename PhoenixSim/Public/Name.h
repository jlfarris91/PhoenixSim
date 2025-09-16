
#pragma once

#include <stdlib.h>

#include "Hashing.h"
#include "DLLExport.h"

namespace Phoenix
{
    struct PHOENIXSIM_API FName
    {
        static const FName None;
        static const FName Empty;
        
        constexpr FName() {}
        constexpr FName(const char* chars, uint32_t len) : Hash(Hashing::FN1VA32(chars, len)) {}
        constexpr FName(hash_t hash) : Hash(hash) {}

        operator hash_t() const;
        bool operator==(const FName& other) const;
        bool operator!=(const FName& other) const;

    private:
        hash_t Hash = 0;
    };

    constexpr FName operator ""_n(const char* chars, size_t len)
    {
        return Hashing::FN1VA32(chars, len);
    }
}
