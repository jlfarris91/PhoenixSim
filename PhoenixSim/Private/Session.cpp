
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

    StartTime = clock();
    CurrTickTime = StartTime;
    LastStepTime = StartTime;
    SPSTimer = StartTime;
    AccTickTime = 0;
}

void Session::Shutdown()
{
    for (const FeatureSharedPtr& feature : FeatureSet->GetFeatures())
    {
        feature->Shutdown();
        feature->Session = nullptr;
    }
}

void Session::QueueAction(const Action& action)
{
    std::lock_guard lock(ActionQueueMutex);
    ActionQueue.emplace_back(SimTime + 1, action);
}

void Session::Tick(const SessionStepArgs& args)
{
    clock_t currTime = clock();
    clock_t dt = currTime - CurrTickTime;
    CurrTickTime = currTime;

    // Skip frames during debug break
    if (dt > CLOCKS_PER_SEC * 3)
    {
        return;
    }

    auto hz = CLOCKS_PER_SEC / args.StepHz;

    AccTickTime += dt;
    while (AccTickTime >= hz)
    {
        clock_t startStepTime = clock();

        Step(args);

        clock_t endStepTime = clock();
        clock_t stepElapsed = endStepTime - startStepTime;

        if (stepElapsed > CLOCKS_PER_SEC * 3)
        {
            break;
        }

        AccTickTime -= std::max(hz, stepElapsed);
        CurrTickTime = clock();
    }

    if (AccTickTime > 0)
    {
        Sleep(static_cast<clock_t>(AccTickTime));
    }

    if (clock() - SPSTimer > CLOCKS_PER_SEC)
    {
        SPSTimer = clock();
        SPS = SimTime - SPSLastSimTime;
        SPSLastSimTime = SimTime;
    }
}

void Session::Step(const SessionStepArgs& args)
{
    LastStepTime = clock();
    SimTime += 1;

    // Process actions
    ProcessActions(SimTime);

    // Step active worlds
    WorldStepArgs worldStepArgs;
    worldStepArgs.SimTime = SimTime;
    worldStepArgs.StepHz = args.StepHz;
    WorldManager->Step(worldStepArgs);
}

clock_t Session::GetCurrTime() const
{
    return CurrTickTime;
}

clock_t Session::GetStartTime() const
{
    return StartTime;
}

clock_t Session::GetLastStepTime() const
{
    return LastStepTime;
}

simtime_t Session::GetSimTime() const
{
    return SimTime;
}

uint64 Session::GetStepsPerSecond() const
{
    return SPS;
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
    // Process incoming actions
    {
        std::lock_guard lock(ActionQueueMutex);

        // Sort action queue by sim time
        std::ranges::sort(ActionQueue,
              [](const TTuple<simtime_t, Action>& a, const TTuple<simtime_t, Action>& b)
              {
                  return std::get<0>(a) < std::get<0>(b);
              });
        
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
