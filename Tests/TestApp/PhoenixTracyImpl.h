
#pragma once

#include "Profiling.h"

namespace Phoenix
{
    namespace Profiling
    {
        struct TracyProfiler : IProfiler
        {
            void BeginZone(const SourceLocation* srcLoc, int32 depth = INDEX_NONE) override;
            void EndZone() override;
            void Text(const char* txt, size_t size) override;
            void TextFmt(const char* fmt, ...) override;
            void Name(const char* txt, size_t size) override;
            void NameFmt(const char* fmt, ...) override;
            void Color(uint32 color) override;
            void Value(uint64 value) override;
        };
    }
}