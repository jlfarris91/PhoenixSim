
// SDL3
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

// Phoenix
#include "Session.h"
#include "Worlds.h"

// Phoenix features
#include "Color.h"
#include "FeatureTrace.h"
#include "FeatureECS.h"
#include "FeatureNavMesh.h"
#include "FeaturePhysics.h"
#include "MortonCode.h"

// SDL impl
#include "SDL/SDLCamera.h"
#include "SDL/SDLDebugRenderer.h"
#include "SDL/SDLDebugState.h"
#include "SDL/SDLViewport.h"

using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;
using namespace Phoenix::Pathfinding;

SDL_Window* GWindow;
SDL_Renderer* GRenderer;

Session* GSession;
bool GSessionThreadWantsExit = false;
std::thread* GSessionThread = nullptr;
std::vector<World> GWorldUpdateQueue;
std::mutex GWorldUpdateQueueMutex;
float GSessionFPS = 0;

SDLDebugState* GDebugState;
SDLDebugRenderer* GDebugRenderer;

SDLCamera* GCamera;
SDLViewport* GViewport;
TOptional<SDL_FPoint> GCameraDragPos;

World* GCurrWorld = nullptr;

void UpdateSessionWorker();
void OnPostWorldUpdate(WorldConstRef world);

void InitSession()
{
    TSharedPtr<FeatureTrace> traceFeature = std::make_shared<FeatureTrace>();
    TSharedPtr<FeatureECS> ecsFeature = std::make_shared<FeatureECS>();
    TSharedPtr<FeatureNavMesh> navMeshFeature = std::make_shared<FeatureNavMesh>();
    TSharedPtr<FeaturePhysics> physicsFeature = std::make_shared<FeaturePhysics>();
    
    SessionCtorArgs sessionArgs;
    sessionArgs.FeatureSetArgs.Features.push_back(traceFeature);
    sessionArgs.FeatureSetArgs.Features.push_back(ecsFeature);
    sessionArgs.FeatureSetArgs.Features.push_back(navMeshFeature);
    sessionArgs.FeatureSetArgs.Features.push_back(physicsFeature);
    sessionArgs.OnPostWorldUpdate = OnPostWorldUpdate;

    GSession = new Session(sessionArgs);

    GSession->Initialize();

    WorldManager* worldManager = GSession->GetWorldManager();

    auto primaryWorld = worldManager->NewWorld("TestWorld"_n);

    GSessionThread = new std::thread(UpdateSessionWorker);
}

void UpdateSessionWorker()
{
    clock_t lastClockTime = 0;
    GSessionThreadWantsExit = false;

    while (!GSessionThreadWantsExit)
    {
        clock_t currClockTime = clock();
        clock_t deltaClockTime = currClockTime - lastClockTime;
        lastClockTime = currClockTime;

        SessionStepArgs stepArgs;
        stepArgs.DeltaTime = deltaClockTime;
        stepArgs.StepHz = 60;

        GSession->Tick(stepArgs);

        GSessionFPS = GSession->GetStepsPerSecond();

        //Sleep(10);
    }
}

void OnPostWorldUpdate(WorldConstRef world)
{
    std::lock_guard lock(GWorldUpdateQueueMutex);
    GWorldUpdateQueue.push_back(world);
}

void DrawGrid();

void OnAppInit(SDL_Window* window, SDL_Renderer* renderer)
{
    InitSession();

    GWindow = window;
    GRenderer = renderer;

    GCamera = new SDLCamera();
    GViewport = new SDLViewport(window, GCamera);

    GDebugState = new SDLDebugState(GViewport);
    GDebugRenderer = new SDLDebugRenderer(renderer, GViewport);
}

void OnAppUpdate()
{
    DrawGrid();

    {
        std::lock_guard lock(GWorldUpdateQueueMutex);

        if (!GWorldUpdateQueue.empty())
        {
            World& world = GWorldUpdateQueue[GWorldUpdateQueue.size() - 1];

            if (!GCurrWorld)
            {
                GCurrWorld = new World(world);
            }
            else
            {
                *GCurrWorld = world;
            }

            GWorldUpdateQueue.clear();
        }
    }

    if (!GCurrWorld)
        return;

    if (GCurrWorld)
    {
        GDebugRenderer->Reset();

        {
            Vec2 bl = Vec2(Distance::Min, Distance::Min);
            Vec2 br = Vec2(Distance::Max, Distance::Min);
            Vec2 tl = Vec2(Distance::Min, Distance::Max);
            Vec2 tr = Vec2(Distance::Max, Distance::Max);
            GDebugRenderer->DrawLine(bl, br, Color::Red);
            GDebugRenderer->DrawLine(br, tr, Color::Red);
            GDebugRenderer->DrawLine(tr, tl, Color::Red);
            GDebugRenderer->DrawLine(tl, bl, Color::Red);
        }

        DrawGrid();

        TArray<FeatureSharedPtr> channelFeatures = GSession->GetFeatureSet()->GetChannelRef(WorldChannels::DebugRender);
        for (const auto& feature : channelFeatures)
        {
            feature->OnDebugRender(*GCurrWorld, *GDebugState, *GDebugRenderer);
        }
    }
}

void DrawGrid()
{
    int32 windowWidth, windowHeight;
    SDL_GetWindowSize(GWindow, &windowWidth, &windowHeight);
    
    auto tl = GViewport->ViewportPosToWorldPos({ 0, 0 });
    auto br = GViewport->ViewportPosToWorldPos({ (float)windowWidth, (float)windowHeight });

    tl.X = Clamp(tl.X, Distance::Min, Distance::Max);
    tl.Y = Clamp(tl.Y, Distance::Min, Distance::Max);
    br.X = Clamp(br.X, Distance::Min, Distance::Max);
    br.Y = Clamp(br.Y, Distance::Min, Distance::Max);

    auto m = Max((float)br.X - (float)tl.X, (float)tl.Y - (float)br.Y);

    int32 step = 1 << MortonCodeGridBits;

    while (GViewport->WorldVecToViewportVec(Vec2(step, 0)).x <= 10)
    {
        step *= 10;
    }

    float minVisStep = step / 10.0f;
    float minVisStepAlpha = GViewport->WorldVecToViewportVec(Vec2(minVisStep, 0)).x;
    minVisStepAlpha = Clamp(minVisStepAlpha / 10.0f, 0.0f, 1.0f);

    int32 steps = int32(m / step);

    m *= 0.5;

    auto minX = (int32)(((float)GCamera->Position.X - m) / step) * step;
    auto minY = (int32)(((float)GCamera->Position.Y - m) / step) * step;

    auto calculateColor = [minVisStepAlpha, step](int32 s, Color& color)
    {
        if (s == 0)
        {
            color = Color::White;
            return;
        }

        int32 a = s;
        int32 n = 0;
        while (a % (step * 10) == 0)
        {
            color *= 1.5;
            a /= 10;
            n++;
            if (a == 0)
                break;
        }

        if (n == 0)
        {
            color.A = uint8(minVisStepAlpha * 255);
        }
        else
        {
            color.A = 255;
        }
    };

    Color color(30, 30, 30);

    int32 a = step;
    while (a > 1)
    {
        a /= 10;
        color *= 1.5;
    }

    for (int32 i = 0; i < steps; ++i)
    {
        Color colorX = color;
        Color colorY = color;

        int32 stepX = minX + i * step;
        calculateColor(stepX, colorX);

        int32 stepY = minY + i * step;
        calculateColor(stepY, colorY);

        Distance x = stepX;
        Distance y = stepY;

        GDebugRenderer->DrawLine(Vec2(x, Distance::Min), Vec2(x, Distance::Max), colorX);
        GDebugRenderer->DrawLine(Vec2(Distance::Min, y), Vec2(Distance::Max, y), colorY);
    }
}

void OnAppShutdown()
{
    GSessionThreadWantsExit = true;
    GSessionThread->join();
}