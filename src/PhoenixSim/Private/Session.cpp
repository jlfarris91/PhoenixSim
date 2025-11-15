
#include "Session.h"
#include "Features.h"
#include "Worlds.h"

#include <algorithm>
#ifdef _WIN32
#include <windows.h>  // For Sleep
#else
#include <unistd.h>   // For usleep
#endif

#include "Profiling.h"

using namespace Phoenix;

Session::Session(const SessionCtorArgs& args)
{
    FeatureSet = std::make_shared<Phoenix::FeatureSet>(args.FeatureSetArgs);

    WorldManagerCtorArgs worldManagerArgs;
    worldManagerArgs.FeatureSet = FeatureSet;
    worldManagerArgs.OnPostWorldUpdate = args.OnPostWorldUpdate;
    WorldManager = std::make_shared<Phoenix::WorldManager>(worldManagerArgs);

    BlockBuffer::CtorArgs sessionBlockArgs;
    for (const FeatureSharedPtr& feature : FeatureSet->GetFeatures())
    {
        const FeatureDefinition& featureDefinition = feature->GetFeatureDefinition();
        for (const BlockBuffer::BlockDefinition& sessionBlock : featureDefinition.SessionBlocks.Definitions)
        {
            sessionBlockArgs.Definitions.push_back(sessionBlock);
        }
    }

    SessionBuffer = MakeUnique<BlockBuffer>(sessionBlockArgs);
}

Session::~Session()
{
    SessionBuffer.release();
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

    StartTime = PHX_SYS_CLOCK_NOW();
    CurrTickTime = StartTime;
    LastStepTime = StartTime;
    AccTickTime = sys_clock_dur_t(0);
    FPSCalc.Reset();
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
    using namespace std::chrono;

    auto currTime = PHX_SYS_CLOCK_NOW();
    auto dt = currTime - CurrTickTime;
    CurrTickTime = currTime;

    // Skip frames during debug break
    if (dt > 3s)
    {
        return;
    }

    sys_clock_dur_t hz = sys_clock_dur_t(1s) / args.StepHz;

    AccTickTime += dt;
    while (AccTickTime >= hz)
    {
        auto startStepTime = PHX_SYS_CLOCK_NOW();

        Step(args);

        sys_clock_t endStepTime = PHX_SYS_CLOCK_NOW();
        auto stepElapsed = endStepTime - startStepTime;

        if (stepElapsed > 3s)
        {
            break;
        }

        AccTickTime -= hz;
        CurrTickTime = PHX_SYS_CLOCK_NOW();
    }
}

void Session::Step(const SessionStepArgs& args)
{
    PHX_PROFILE_ZONE_SCOPED;

    LastStepTime = PHX_SYS_CLOCK_NOW();
    SimTime += 1;

    FPSCalc.Tick();

    // Process actions
    ProcessActions(SimTime);

    // Step features at the session level
    UpdateSession(SimTime, args.StepHz);

    // Step active worlds
    WorldStepArgs worldStepArgs;
    worldStepArgs.SimTime = SimTime;
    worldStepArgs.StepHz = args.StepHz;
    WorldManager->Step(worldStepArgs);
}

BlockBuffer* Session::GetBuffer()
{
    return SessionBuffer.get();
}

const BlockBuffer* Session::GetBuffer() const
{
    return SessionBuffer.get();
}

sys_clock_t Session::GetCurrTime() const
{
    return CurrTickTime;
}

sys_clock_t Session::GetStartTime() const
{
    return StartTime;
}

sys_clock_t Session::GetLastStepTime() const
{
    return LastStepTime;
}

simtime_t Session::GetSimTime() const
{
    return SimTime;
}

const FPSCalc& Session::GetFPSCalc() const
{
    return FPSCalc;
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
    PHX_PROFILE_ZONE_SCOPED;

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

void Session::UpdateSession(simtime_t time, uint32 stepHz) const
{
    FeatureUpdateArgs updateArgs;
    updateArgs.SimTime = time;
    updateArgs.StepHz = stepHz;

    // Pre-update
    {
        PHX_PROFILE_ZONE_SCOPED_N("PreUpdate");
        const TArray<FeatureSharedPtr>& channelFeatures = FeatureSet->GetChannelRef(FeatureChannels::PreUpdate);
        for (const FeatureSharedPtr& feature : channelFeatures)
        {
            feature->OnPreUpdate(updateArgs);
        }
    }

    // Update
    {
        PHX_PROFILE_ZONE_SCOPED_N("Update");
        const TArray<FeatureSharedPtr>& channelFeatures = FeatureSet->GetChannelRef(FeatureChannels::Update);
        for (const FeatureSharedPtr& feature : channelFeatures)
        {
            feature->OnUpdate(updateArgs);
        }
    }

    // Post-update
    {
        PHX_PROFILE_ZONE_SCOPED_N("PostUpdate");
        const TArray<FeatureSharedPtr>& channelFeatures = FeatureSet->GetChannelRef(FeatureChannels::PostUpdate);
        for (const FeatureSharedPtr& feature : channelFeatures)
        {
            feature->OnPostUpdate(updateArgs);
        }
    }
}
