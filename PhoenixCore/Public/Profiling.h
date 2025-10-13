
#pragma once

#include "PhoenixCore.h"
#include "PlatformTypes.h"

namespace Phoenix
{
    namespace Profiling
    {
        struct PHOENIXCORE_API SourceLocation
        {
            const char* Name;
            const char* Function;
            const char* File;
            uint32 Line;
            uint32 Color;
        };

        struct PHOENIXCORE_API IProfiler
        {
            virtual ~IProfiler() = default;

            virtual void BeginZone(const SourceLocation* srcLoc, int32 depth = INDEX_NONE) = 0;
            virtual void EndZone() = 0;
            virtual void Text(const char* txt, size_t size) = 0;
            virtual void TextFmt(const char* fmt, ...) = 0;
            virtual void Name(const char* txt, size_t size) = 0;
            virtual void NameFmt(const char* fmt, ...) = 0;
            virtual void Color(uint32 color) = 0;
            virtual void Value(uint64 value) = 0;
        };

        PHOENIXCORE_API bool HasProfiler();
        PHOENIXCORE_API IProfiler& GetProfiler();
        PHOENIXCORE_API void SetProfiler(IProfiler* profiler);

        struct PHOENIXCORE_API ScopedZone
        {
            PHX_FORCE_INLINE ScopedZone(const SourceLocation* srcLoc, int32 depth = INDEX_NONE)
            {
                GetProfiler().BeginZone(srcLoc, depth);
            }

            PHX_FORCE_INLINE ~ScopedZone()
            {
                GetProfiler().EndZone();
            }

            PHX_FORCE_INLINE void Text(const char* txt, size_t size)
            {
                GetProfiler().Text(txt, size);
            }

            template <class ...TArgs>
            PHX_FORCE_INLINE void TextFmt(const char* fmt, TArgs&& ...)
            {
                GetProfiler().TextFmt(fmt, std::forward<TArgs>()...);
            }

            PHX_FORCE_INLINE void Name(const char* txt, size_t size)
            {
                GetProfiler().Name(txt, size);
            }

            template <class ...TArgs>
            PHX_FORCE_INLINE void NameFmt(const char* fmt, TArgs&& ...)
            {
                GetProfiler().NameFmt(fmt, std::forward<TArgs>()...);
            }

            PHX_FORCE_INLINE void Color(uint32 color)
            {
                GetProfiler().Color(color);
            }

            PHX_FORCE_INLINE void Value(uint64 value)
            {
                GetProfiler().Value(value);
            }
        };
    }
}

#ifndef PHX_PROFILE_CALLSTACK
#define PHX_PROFILE_CALLSTACK 0
#endif

#ifdef PHX_PROFILE_ENABLE

#define PHX_PROFILE_ZONE(varname) \
    static constexpr Phoenix::Profiling::SourceLocation PHX_CONCAT(__source_loc, PHX_LINE) \
    { nullptr, PHX_FUNCTION, PHX_FILE, (uint32_t)PHX_LINE, 0 }; \
    Phoenix::Profiling::ScopedZone varname(&PHX_CONCAT(__source_loc, PHX_LINE), PHX_PROFILE_CALLSTACK)

#define PHX_PROFILE_ZONE_N(varname, name) \
    static constexpr Phoenix::Profiling::SourceLocation PHX_CONCAT(__source_loc, PHX_LINE) \
    { name, PHX_FUNCTION, PHX_FILE, (uint32_t)PHX_LINE, 0 }; \
    Phoenix::Profiling::ScopedZone varname(&PHX_CONCAT(__source_loc, PHX_LINE), PHX_PROFILE_CALLSTACK)

#define PHX_PROFILE_ZONE_C(varname, color) \
    static constexpr Phoenix::Profiling::SourceLocation PHX_CONCAT(__source_loc, PHX_LINE) \
    { nullptr, PHX_FUNCTION, PHX_FILE, (uint32_t)PHX_LINE, (uint32_t)color }; \
    Phoenix::Profiling::ScopedZone varname(&PHX_CONCAT(__source_loc, PHX_LINE), PHX_PROFILE_CALLSTACK)

#define PHX_PROFILE_ZONE_NC(varname, name, color) \
    static constexpr Phoenix::Profiling::SourceLocation PHX_CONCAT(__source_loc, PHX_LINE) \
    { name, PHX_FUNCTION, PHX_FILE, (uint32_t)PHX_LINE, (uint32_t)color }; \
    Phoenix::Profiling::ScopedZone varname(&PHX_CONCAT(__source_loc, PHX_LINE), PHX_PROFILE_CALLSTACK)

#if defined(_MSC_VER) 
#   define PHX_SUPPRESS_VAR_SHADOW_WARNING(expr) \
        _Pragma("warning(push)") \
        _Pragma("warning(disable : 4456)") \
        expr \
        _Pragma("warning(pop)")
#else
#   define PHX_SUPPRESS_VAR_SHADOW_WARNING(expr) expr
#endif

#define PHX_PROFILE_ZONE_SCOPED PHX_SUPPRESS_VAR_SHADOW_WARNING( PHX_PROFILE_ZONE(__scoped_zone) )
#define PHX_PROFILE_ZONE_SCOPED_N(name) PHX_SUPPRESS_VAR_SHADOW_WARNING( PHX_PROFILE_ZONE_N(__scoped_zone, name) )
#define PHX_PROFILE_ZONE_SCOPED_C(color) PHX_SUPPRESS_VAR_SHADOW_WARNING( PHX_PROFILE_ZONE_C(__scoped_zone, color) )
#define PHX_PROFILE_ZONE_SCOPED_NC(name, color) PHX_SUPPRESS_VAR_SHADOW_WARNING( PHX_PROFILE_ZONE_NC(__scoped_zone, name, color) )

#define PHX_PROFILE_ZONE_TEXT(txt, size) __scoped_zone.Text(txt, size)
#define PHX_PROFILE_ZONE_TEXTF(fmt, ...) __scoped_zone.TextFmt(fmt, ...)
#define PHX_PROFILE_ZONE_NAME(txt, size) __scoped_zone.Name(txt, size)
#define PHX_PROFILE_ZONE_NAMEF(fmt, ...) __scoped_zone.NameFmt(fmt, ...)
#define PHX_PROFILE_ZONE_COLOR(color) __scoped_zone.Color(color)
#define PHX_PROFILE_ZONE_VALUE(value) __scoped_zone.Value(value)

#else

#define PHX_PROFILE_ZONE
#define PHX_PROFILE_ZONE_N
#define PHX_PROFILE_ZONE_C
#define PHX_PROFILE_ZONE_NC

#define PHX_PROFILE_ZONE_SCOPED
#define PHX_PROFILE_ZONE_SCOPED_N
#define PHX_PROFILE_ZONE_SCOPED_C
#define PHX_PROFILE_ZONE_SCOPED_NC

#define PHX_PROFILE_ZONE_TEXT
#define PHX_PROFILE_ZONE_TEXTF
#define PHX_PROFILE_ZONE_NAME
#define PHX_PROFILE_ZONE_NAMEF
#define PHX_PROFILE_ZONE_COLOR
#define PHX_PROFILE_ZONE_VALUE

#endif