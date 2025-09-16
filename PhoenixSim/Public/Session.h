#pragma once

#include <shared_mutex>

#include "Features.h"
#include "PhoenixSim.h"

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
        dt_t DeltaTime = 0.0f;

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

        void QueueAction(simtime_t time, const Action& action);

        void Step(const SessionStepArgs& args);

        FeatureSet* GetFeatureSet() const;
        WorldManager* GetWorldManager() const;

    private:

        void ProcessActions(simtime_t time);

        TSharedPtr<FeatureSet> FeatureSet;
        TSharedPtr<WorldManager> WorldManager;

        TArray<TTuple<simtime_t, Action>> ActionQueue;
        std::shared_mutex ActionQueueMutex;
    };
}

