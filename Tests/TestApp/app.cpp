
#include <ranges>

// Tracy
#include "PhoenixTracyImpl.h"
#include <tracy/Tracy.hpp>

// ImGui
#include "imgui.h"
#include "imgui_internal.h"

// SDL3
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_timer.h>

// Phoenix
#include "Session.h"
#include "Worlds.h"
#include "Color.h"
#include "MortonCode.h"

// Phoenix features
#include "FeatureECS.h"
#include "FeatureNavMesh.h"
#include "FeaturePhysics.h"
#include "FeatureLua.h"

// SDL impl
#include "SDL/SDLCamera.h"
#include "SDL/SDLDebugRenderer.h"
#include "SDL/SDLDebugState.h"
#include "SDL/SDLTool.h"
#include "SDL/SDLViewport.h"

// Test App Tools
#include "BodyComponent.h"
#include "Tools/CameraTool.h"
#include "Tools/EntityTool.h"
#include "Tools/ImGuiPropertyGrid.h"
#include "Tools/NavMeshTool.h"

using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;
using namespace Phoenix::Pathfinding;

SDL_Window* GWindow;
SDL_Renderer* GRenderer;

uint32 GRendererFPSCounter = 0;
uint64 GRendererFPSTimer = 0;
float GRendererFPS = 0.0f;

Profiling::TracyProfiler GTracyProfiler;

Session* GSession;
bool GSessionThreadWantsExit = false;
std::thread* GSessionThread = nullptr;
World* GLatestWorldView = nullptr;
std::mutex GWorldViewUpdateMutex;
float GSessionFPS = 0;

SDLDebugState* GDebugState;
SDLDebugRenderer* GDebugRenderer;

SDLCamera* GCamera;
SDLViewport* GViewport;

TArray<TSharedPtr<ISDLTool>> Tools;
TArray<TSharedPtr<ISDLTool>> ActiveTools;

World* GCurrWorldView = nullptr;

struct EntityBodyShape
{
    Transform2D Transform;
    Distance Radius;
    SDL_Color Color;
    uint64 ZCode;
    Distance VelLen;
};

std::vector<EntityBodyShape> GEntityBodies;

void UpdateSessionWorker();
void OnPostWorldUpdate(WorldConstRef world);

void InitSession()
{
    TSharedPtr<FeatureECS> ecsFeature = std::make_shared<FeatureECS>();
    TSharedPtr<FeatureNavMesh> navMeshFeature = std::make_shared<FeatureNavMesh>();
    TSharedPtr<FeaturePhysics> physicsFeature = std::make_shared<FeaturePhysics>();
    TSharedPtr<FeatureLua> luaFeature = std::make_shared<FeatureLua>();
    
    SessionCtorArgs sessionArgs;
    sessionArgs.FeatureSetArgs.Features.push_back(ecsFeature);
    sessionArgs.FeatureSetArgs.Features.push_back(navMeshFeature);
    sessionArgs.FeatureSetArgs.Features.push_back(physicsFeature);
    sessionArgs.FeatureSetArgs.Features.push_back(luaFeature);
    sessionArgs.OnPostWorldUpdate = OnPostWorldUpdate;

    GSession = new Session(sessionArgs);

    GSession->Initialize();

    WorldManager* worldManager = GSession->GetWorldManager();

    auto primaryWorld = worldManager->NewWorld("TestWorld"_n);

    FeatureECS::RegisterArchetypeDefinition<TransformComponent, BodyComponent>(*primaryWorld, "Unit"_n);
    
    GSessionThread = new std::thread(UpdateSessionWorker);
}

void UpdateSessionWorker()
{
    PHX_PROFILE_SET_THREAD_NAME("Sim", 0);

    clock_t lastClockTime = 0;
    GSessionThreadWantsExit = false;

    while (!GSessionThreadWantsExit)
    {
        FrameMarkNamed("Sim");

        clock_t currClockTime = PHX_CLOCK();
        clock_t deltaClockTime = currClockTime - lastClockTime;
        lastClockTime = currClockTime;

        SessionStepArgs stepArgs;
        stepArgs.DeltaTime = deltaClockTime;
        stepArgs.StepHz = 60;

        GSession->Tick(stepArgs);

        GSessionFPS = static_cast<float>(GSession->GetFramerate());

        //Sleep(10);
    }
}

void OnPostWorldUpdate(WorldConstRef world)
{
    PHX_PROFILE_ZONE_SCOPED;

    std::lock_guard lock(GWorldViewUpdateMutex);

    if (!GLatestWorldView)
    {
        GLatestWorldView = new World(world);
    }
    else
    {
        *GLatestWorldView = world;
    }
}

void DrawGrid();

void OnAppInit(SDL_Window* window, SDL_Renderer* renderer)
{
    SetProfiler(&GTracyProfiler);

    InitSession();

    GWindow = window;
    GRenderer = renderer;

    GCamera = new SDLCamera();
    GViewport = new SDLViewport(window, GCamera);

    GDebugState = new SDLDebugState(GViewport);
    GDebugRenderer = new SDLDebugRenderer(renderer, GViewport);

    auto cameraTool = MakeShared<CameraTool>(GSession, GCamera, GViewport);
    auto entityTool = MakeShared<EntityTool>(GSession);
    auto navMeshTool = MakeShared<NavMeshTool>(GSession);

    Tools.push_back(cameraTool);
    Tools.push_back(entityTool);
    Tools.push_back(navMeshTool);

    ActiveTools.push_back(cameraTool);
    ActiveTools.push_back(entityTool);
    ActiveTools.push_back(navMeshTool);
}

void OnAppRenderWorld()
{
    float mx, my;
    SDL_GetMouseState(&mx, &my);

    ++GRendererFPSCounter;
    Uint64 currTicks = SDL_GetTicks();
    unsigned long long tickDelta = currTicks - GRendererFPSTimer;
    if (tickDelta > CLOCKS_PER_SEC)
    {
        GRendererFPSTimer = currTicks;
        GRendererFPS = static_cast<float>(GRendererFPSCounter);
        GRendererFPS += static_cast<float>(tickDelta) / CLOCKS_PER_SEC;
        GRendererFPSCounter = 0;
    }

    GDebugRenderer->Reset();

    {
        std::lock_guard lock(GWorldViewUpdateMutex);

        if (GLatestWorldView)
        {
            if (!GCurrWorldView)
            {
                GCurrWorldView = new World(*GLatestWorldView);
            }
            else
            {
                *GCurrWorldView = *GLatestWorldView;
            }

            GEntityBodies.clear();

            FeatureECS::Entities(*GCurrWorldView)
                .ForEachEntity(TFunction([](const EntityComponentSpan<const TransformComponent&, const BodyComponent&>& span)
                {
                    for (auto && [entity, transformComp, bodyComp] : span)
                    {
                        EntityBodyShape entityBodyShape;
                        entityBodyShape.Transform = transformComp.Transform;
                        entityBodyShape.Radius = bodyComp.Radius;
                        entityBodyShape.Color = SDL_Color(0, 255, 0);
                        entityBodyShape.ZCode = transformComp.ZCode;
                        entityBodyShape.VelLen = bodyComp.LinearVelocity.Length();

                        if (!HasAnyFlags(bodyComp.Flags, EBodyFlags::Awake))
                        {
                            entityBodyShape.Color = SDL_Color(0, 128, 0);
                        }

                        if (bodyComp.Movement == EBodyMovement::Attached &&
                            transformComp.AttachParent != EntityId::Invalid)
                        {
                            if (TransformComponent* parentTransformComp = FeatureECS::GetComponent<TransformComponent>(*GCurrWorldView, transformComp.AttachParent))
                            {
                                entityBodyShape.Transform.Position = parentTransformComp->Transform.Position + entityBodyShape.Transform.Position.Rotate(parentTransformComp->Transform.Rotation);
                                entityBodyShape.Transform.Rotation += parentTransformComp->Transform.Rotation;
                            }
                        }

                        GEntityBodies.push_back(entityBodyShape);
                    }
                }));
        }
    }

    if (!GCurrWorldView)
        return;

    DrawGrid();

    // Draw distance value bounds
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

    for (const EntityBodyShape& entityBodyShape : GEntityBodies)
    {
        Vec2 pt1 = Vec2::XAxis * entityBodyShape.Radius;
        Vec2 pt2 = pt1.Rotate(Deg2Rad(-135));
        Vec2 pt3 = pt1.Rotate(Deg2Rad(135));

        Transform2D transform(Vec2::Zero, entityBodyShape.Transform.Rotation, 1.0f);
        pt1 = entityBodyShape.Transform.Position + transform.RotateVector(pt1);
        pt2 = entityBodyShape.Transform.Position + transform.RotateVector(pt2);
        pt3 = entityBodyShape.Transform.Position + transform.RotateVector(pt3);

        Vec2 points[4] = { pt1, pt2, pt3, pt1 };

        Color color(entityBodyShape.Color.r, entityBodyShape.Color.g, entityBodyShape.Color.b);
        GDebugRenderer->DrawLines(points, 4, color);
    }

    // Let features draw to the renderer
    TArray<FeatureSharedPtr> channelFeatures = GSession->GetFeatureSet()->GetChannelRef(FeatureChannels::DebugRender);
    for (const auto& feature : channelFeatures)
    {
        feature->OnDebugRender(*GCurrWorldView, *GDebugState, *GDebugRenderer);
    }

    for (const TSharedPtr<ISDLTool>& tool : ActiveTools)
    {
        tool->OnAppRenderWorld(*GCurrWorldView, *GDebugState, *GDebugRenderer);
    }
}

void OnAppRenderUI()
{
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::Begin("Debug"))
    {
        ImGui::Text("Sim FPS:"); ImGui::SameLine(100); ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / GSession->GetFramerate(), GSession->GetFramerate());
        ImGui::Text("SDL FPS:"); ImGui::SameLine(100); ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / GRendererFPS, GRendererFPS);
        ImGui::Text("ImGui FPS:"); ImGui::SameLine(100); ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        if (GCurrWorldView)
        {
            const FeatureECSDynamicBlock& ecsDynamicBlock = GCurrWorldView->GetBlockRef<FeatureECSDynamicBlock>();

            ImGui::Text("ECS: E:%zu T:%zu G:%zu",
                ecsDynamicBlock.Entities.Num(),
                ecsDynamicBlock.Tags.Num(),
                ecsDynamicBlock.Groups.Num());
        }

        if (ImGui::CollapsingHeader("Features"))
        {
            for (const auto& feature : GSession->GetFeatureSet()->GetFeatures())
            {
                const auto& featureDefinition = feature->GetFeatureDefinition();
                if (ImGui::CollapsingHeader(featureDefinition.DisplayName))
                {
                    DrawPropertyGrid(feature.get(), featureDefinition);
                }
            }
        }

        ImGui::End();
    }

    if (ImGui::Begin("Tools"))
    {
        for (const auto& tool : Tools)
        {
            const auto& descriptor = tool->GetTypeDescriptor();

            if (ImGui::CollapsingHeader(descriptor.DisplayName))
            {
                DrawPropertyGrid(tool.get(), descriptor);
            }
        }

        ImGui::End();
    }
    
    // if (ImGui::Begin("ECS"))
    // {
    //     if (GCurrWorldView)
    //     {
    //         const FeatureECSDynamicBlock& ecsDynamicBlock = GCurrWorldView->GetBlockRef<FeatureECSDynamicBlock>();
    //
    //         ImGui::Text("ECS: E:%zu T:%zu G:%zu",
    //             ecsDynamicBlock.Entities.Num(),
    //             ecsDynamicBlock.Tags.Num(),
    //             ecsDynamicBlock.Groups.Num());
    //
    //         if (ImGui::TreeNode("Archetype Lists"))
    //         {
    //
    //             ecsDynamicBlock.ArchetypeManager.
    //
    //             ImGui::TreePop();
    //         }
    //         
    //     }
    //
    //     ImGui::End();
    // }
}

void OnAppEvent(SDL_Event* event)
{
    GDebugState->ProcessAppEvent(event);

    for (const TSharedPtr<ISDLTool>& tool : ActiveTools)
    {
        tool->OnAppEvent(*GDebugState, event);
    }
}

void OnAppShutdown()
{
    GSessionThreadWantsExit = true;
    GSessionThread->join();
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