#pragma once

#include <shared_mutex>

#include "Features.h"
#include "FPSCalc.h"

namespace Phoenix
{
    class WorldManager;
}

namespace Phoenix
{
    struct PHOENIXSIM_API SessionCtorArgs
    {
        FeatureSetCtorArgs FeatureSetArgs;
        PostWorldUpdateDelegate OnPostWorldUpdate;
    };

    struct PHOENIXSIM_API SessionStepArgs
    {
        uint32 StepHz = 60;

        // Optionally only step this world.
        FName WorldName = FName::None;
    };

    class PHOENIXSIM_API Session
    {
    public:

        Session(const SessionCtorArgs& args);
        ~Session();

        void Initialize();
        void Shutdown();

        void QueueAction(const Action& action);

        void Tick(const SessionStepArgs& args);
        void Step(const SessionStepArgs& args);

        BlockBuffer* GetBuffer();
        const BlockBuffer* GetBuffer() const;

        sys_clock_t GetCurrTime() const;
        sys_clock_t GetStartTime() const;
        sys_clock_t GetLastStepTime() const;
        simtime_t GetSimTime() const;
        const FPSCalc& GetFPSCalc() const;

        FeatureSet* GetFeatureSet() const;
        WorldManager* GetWorldManager() const;

    private:

        void ProcessActions(simtime_t time);

        void UpdateSession(simtime_t time, uint32 stepHz) const;

        TSharedPtr<FeatureSet> FeatureSet;
        TSharedPtr<WorldManager> WorldManager;

        TArray<TTuple<simtime_t, Action>> ActionQueue;
        std::shared_mutex ActionQueueMutex;

        sys_clock_t StartTime;
        sys_clock_t CurrTickTime;
        sys_clock_dur_t AccTickTime = sys_clock_dur_t(0);
        sys_clock_t LastStepTime;
        simtime_t SimTime = 0;

        // Steps per second
        FPSCalc FPSCalc;

        TUniquePtr<BlockBuffer> SessionBuffer;
    };

    using SessionPtr = Session*;
    using SessionConstPtr = const Session*;
    using SessionRef = Session&;
    using SessionConstRef = const Session&;
}

