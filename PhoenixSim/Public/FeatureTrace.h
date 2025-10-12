
#pragma once

#include "Features.h"
#include "Containers/FixedArray.h"

namespace Phoenix
{
    constexpr size_t MAX_TRACE_EVENTS = 100000;

    enum class ETraceFlags
    {
        None = 0,
        Begin = 1,
        End = 2,
        Counter = 4,
    };
    
    struct PHOENIXSIM_API TraceEvent
    {
        FName Name;
        FName Id;
        ETraceFlags Flag = ETraceFlags::None;
        clock_t Time = 0;
        int32 Counter = 0;
    };

    struct PHOENIXSIM_API FeatureTraceScratchBlock
    {
        DECLARE_WORLD_BLOCK_SCRATCH(FeatureTraceScratchBlock)

        TFixedArray<TraceEvent, MAX_TRACE_EVENTS> Events;
    };
    
    class PHOENIXSIM_API FeatureTrace : public IFeature
    {
        FEATURE_BEGIN(FeatureTrace)
            FEATURE_BLOCK(FeatureTraceScratchBlock)
        FEATURE_END()

    public:

        FeatureTrace();
        
        // Use ScopedTrace instead
        static void PushTrace(WorldRef world, FName name, FName id, ETraceFlags flags, int32 counter = 0);
    };

    struct PHOENIXSIM_API ScopedTrace
    {
        ScopedTrace(WorldRef world, const FName& name, const FName& id = {})
            : World(world)
            , Name(name)
            , Id(id)
        {
            FeatureTrace::PushTrace(world, name, id, ETraceFlags::Begin, Counter);
        }
        ~ScopedTrace()
        {
            FeatureTrace::PushTrace(World, Name, Id, ETraceFlags::End, Counter);
        }
        WorldRef World;
        FName Name;
        FName Id;
        int32 Counter = 0;
    };
}
