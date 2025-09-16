
#include "Session.h"
#include "Features.h"
#include "Worlds.h"

#include <algorithm>

using namespace Phoenix;

Session::Session(const SessionCtorArgs& args)
{
    FeatureSet = std::make_shared<Phoenix::FeatureSet>(args.FeatureSetArgs);

    WorldManagerCtorArgs worldManagerArgs;
    worldManagerArgs.FeatureSet = FeatureSet;
    worldManagerArgs.OnPostWorldUpdate = args.OnPostWorldUpdate;
    WorldManager = std::make_shared<Phoenix::WorldManager>(worldManagerArgs);
}

Session::~Session()
{
    FeatureSet.reset();
    WorldManager.reset();
}

void Session::Initialize()
{
    for (const FeatureSharedPtr& feature : FeatureSet->GetFeatures())
    {
        feature->Session = this;
        feature->Initialize();
    }
}

void Session::Shutdown()
{
    for (const FeatureSharedPtr& feature : FeatureSet->GetFeatures())
    {
        feature->Shutdown();
        feature->Session = nullptr;
    }
}

void Session::QueueAction(simtime_t time, const Action& action)
{
    std::lock_guard lock(ActionQueueMutex);
    ActionQueue.emplace_back(time, action);
}

void Session::Step(const SessionStepArgs& args)
{
    simtime_t simTime = 0;
    
    // Process actions
    ProcessActions(simTime);

    // Step active worlds
    WorldStepArgs worldStepArgs;
    worldStepArgs.SimTime = simTime;
    WorldManager->Step(worldStepArgs);
}

FeatureSet* Session::GetFeatureSet() const
{
    return FeatureSet.get();
}

WorldManager* Session::GetWorldManager() const
{
    return WorldManager.get();
}

void Session::ProcessActions(simtime_t time)
{
    // Sort action queue by sim time
    std::sort(ActionQueue.begin(), ActionQueue.end(),
        [](const TTuple<simtime_t, Action>& a, const TTuple<simtime_t, Action>& b)
        {
            return std::get<0>(a) < std::get<0>(b);
        });

    // Process incoming actions
    {
        std::lock_guard lock(ActionQueueMutex);
        
        auto iter = ActionQueue.begin();
        for (; iter != ActionQueue.end(); ++iter)
        {
            const simtime_t& timestamp = std::get<0>(*iter);
            const Action& action = std::get<1>(*iter);

            // An action came in with a timestamp further along than the sim
            if (timestamp < time)
            {
                // This should probably result in a desync
                continue;
            }

            // 
            if (timestamp > time)
            {
                break;
            }

            WorldSendActionArgs args;
            args.Action = action;
            WorldManager->SendAction(args);
        }

        if (iter != ActionQueue.begin())
        {
            ActionQueue.erase(ActionQueue.begin(), iter);
        }
    }
}
