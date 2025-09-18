
#pragma once

#include "Features.h"
#include "FixedArray.h"

namespace Phoenix
{
    constexpr size_t MAX_TRACE_EVENTS = 100000;

    enum class ETraceFlags
    {
        None = 0,
        Begin = 1,
        End = 2,
    };
    
    struct PHOENIXSIM_API TraceEvent
    {
        FName Name;
        FName Id;
        ETraceFlags Flag = ETraceFlags::None;
        clock_t Time = 0;
    };

    struct PHOENIXSIM_API FeatureTraceScratchBlock
    {
        static constexpr FName StaticName = "FeatureTraceScratchBlock"_n;

        TFixedArray<TraceEvent, MAX_TRACE_EVENTS> Events;
    };
    
    class PHOENIXSIM_API FeatureTrace : public IFeature
    {
    public:

        static constexpr FName StaticName = "FeatureTrace"_n;

        FeatureTrace();

        FName GetName() const override;

        FeatureDefinition GetFeatureDefinition() override;
        
        // Use ScopedTrace instead
        static void PushTrace(WorldRef world, FName name, FName id, ETraceFlags flags);

    private:

        FeatureDefinition FeatureDefinition;
    };

    struct PHOENIXSIM_API ScopedTrace
    {
        ScopedTrace(WorldRef world, const FName& name, const FName& id = {})
            : World(world)
            , Name(name)
            , Id(id)
        {
            FeatureTrace::PushTrace(world, name, id, ETraceFlags::Begin);
        }
        ~ScopedTrace()
        {
            FeatureTrace::PushTrace(World, Name, Id, ETraceFlags::End);
        }
        WorldRef World;
        FName Name;
        FName Id;
    };
}
