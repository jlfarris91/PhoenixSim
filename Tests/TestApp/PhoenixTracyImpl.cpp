
#include "PhoenixTracyImpl.h"
#include "Tracy.hpp"

using namespace tracy;

void Phoenix::Profiling::TracyProfiler::SetThreadName(const char* txt, int32_t hint)
{
    SetThreadNameWithHint(txt, hint);
}

void Phoenix::Profiling::TracyProfiler::BeginZone(const SourceLocation* srcLoc, int32 depth)
{
    auto zoneQueue = QueueType::ZoneBegin;
    if( depth > 0 && has_callstack() )
    {
        tracy::GetProfiler().SendCallstack( depth );
        zoneQueue = QueueType::ZoneBeginCallstack;
    }
    TracyQueuePrepare( zoneQueue );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)srcLoc );
    TracyQueueCommit( zoneBeginThread );
}

void Phoenix::Profiling::TracyProfiler::EndZone()
{
    TracyQueuePrepare( QueueType::ZoneEnd );
    MemWrite( &item->zoneEnd.time, Profiler::GetTime() );
    TracyQueueCommit( zoneEndThread );
}

void Phoenix::Profiling::TracyProfiler::Text(const char* txt, size_t size)
{
    assert( size < (std::numeric_limits<uint16_t>::max)() );
    auto ptr = (char*)tracy_malloc( size );
    memcpy( ptr, txt, size );
    TracyQueuePrepare( QueueType::ZoneText );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyQueueCommit( zoneTextFatThread );
}

void Phoenix::Profiling::TracyProfiler::TextFmt(const char* fmt, ...)
{
    va_list args;
    va_start( args, fmt );
    auto size = vsnprintf( nullptr, 0, fmt, args );
    va_end( args );
    if( size < 0 ) return;
    assert( size < (std::numeric_limits<uint16_t>::max)() );

    char* ptr = (char*)tracy_malloc( size_t( size ) + 1 );
    va_start( args, fmt );
    vsnprintf( ptr, size_t( size ) + 1, fmt, args );
    va_end( args );

    TracyQueuePrepare( QueueType::ZoneText );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyQueueCommit( zoneTextFatThread );
}

void Phoenix::Profiling::TracyProfiler::Name(const char* txt, size_t size)
{
    auto ptr = (char*)tracy_malloc( size );
    memcpy( ptr, txt, size );
    TracyQueuePrepare( QueueType::ZoneName );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyQueueCommit( zoneTextFatThread );
}

void Phoenix::Profiling::TracyProfiler::NameFmt(const char* fmt, ...)
{
    va_list args;
    va_start( args, fmt );
    auto size = vsnprintf( nullptr, 0, fmt, args );
    va_end( args );
    if( size < 0 ) return;
    assert( size < (std::numeric_limits<uint16_t>::max)() );

    char* ptr = (char*)tracy_malloc( size_t( size ) + 1 );
    va_start( args, fmt );
    vsnprintf( ptr, size_t( size ) + 1, fmt, args );
    va_end( args );

    TracyQueuePrepare( QueueType::ZoneName );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyQueueCommit( zoneTextFatThread );
}

void Phoenix::Profiling::TracyProfiler::Color(uint32 color)
{
    TracyQueuePrepare( QueueType::ZoneColor );
    MemWrite( &item->zoneColor.b, uint8_t( ( color       ) & 0xFF ) );
    MemWrite( &item->zoneColor.g, uint8_t( ( color >> 8  ) & 0xFF ) );
    MemWrite( &item->zoneColor.r, uint8_t( ( color >> 16 ) & 0xFF ) );
    TracyQueueCommit( zoneColorThread );
}

void Phoenix::Profiling::TracyProfiler::Value(uint64 value)
{
    TracyQueuePrepare( QueueType::ZoneValue );
    MemWrite( &item->zoneValue.value, value );
    TracyQueueCommit( zoneValueThread );
}
