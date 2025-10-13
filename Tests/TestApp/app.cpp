
#include <ranges>

// Tracy
#include "PhoenixTracyImpl.h"

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

uint32 GRendererFPSCounter = 0;
uint64 GRendererFPSTimer = 0;
float GRendererFPS = 0.0f;

Profiling::TracyProfiler GTracyProfiler;

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

struct EntityBodyShape
{
    Transform2D Transform;
    Distance Radius;
    SDL_Color Color;
    uint64 ZCode;
    Distance VelLen;
};

std::vector<EntityBodyShape> GEntityBodies;

struct TraceEntry
{
    FName Name;
    FName Id;
    PHXString DisplayName;
    clock_t StartTime;
    clock_t Duration;
    uint32 NumChildren = 0;
};

struct TraceFrame
{
    uint32 Frame = 0;
    clock_t TimeStamp = 0;
    clock_t Duration = 0;
    TArray<TraceEntry> Entries;
};

TFixedQueue<TraceFrame, 60> GTraceFrames;

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
    SetProfiler(&GTracyProfiler);

    InitSession();

    GWindow = window;
    GRenderer = renderer;

    GCamera = new SDLCamera();
    GViewport = new SDLViewport(window, GCamera);

    GDebugState = new SDLDebugState(GViewport);
    GDebugRenderer = new SDLDebugRenderer(renderer, GViewport);
}

void OnAppRenderWorld()
{
    float mx, my;
    SDL_GetMouseState(&mx, &my);

    Vec2 mouseWorldPos = GViewport->ViewportPosToWorldPos({mx, my});

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
        std::lock_guard lock(GWorldUpdateQueueMutex);

        if (!GWorldUpdateQueue.empty())
        {
            for (const World& world : GWorldUpdateQueue)
            {
                const FeatureTraceScratchBlock* traceBlock = world.GetBlock<FeatureTraceScratchBlock>();
                if (!traceBlock)
                    continue;

                TraceFrame frame;
                frame.Frame = world.GetBlockRef<WorldDynamicBlock>().SimTime;

                if (!traceBlock->Events.IsEmpty())
                    frame.TimeStamp = traceBlock->Events[0].Time;

                TArray<uint32> stack;

                for (const auto& event : traceBlock->Events)
                {
                    TraceEntry* entry = nullptr;

                    if (event.Flag == ETraceFlags::Begin)
                    {
                        entry = &frame.Entries.emplace_back();
                        entry->Name = event.Name;
                        entry->Id = event.Id;
                        entry->Duration = 0;
                        entry->StartTime = event.Time;

                        char name[256];
                        if (entry->Id == FName::None)
                        {
                            sprintf_s(name, _countof(name), "%s", event.Name.Debug);
                        }
                        else
                        {
                            sprintf_s(name, _countof(name), "%s:%s", event.Name.Debug, event.Id.Debug);
                        }
                        entry->DisplayName = name;

                        if (!stack.empty())
                        {
                            frame.Entries[stack.back()].NumChildren++;
                        }

                        stack.push_back(frame.Entries.size() - 1);
                    }

                    if (event.Flag == ETraceFlags::End)
                    {
                        for (int32 i = frame.Entries.size() - 1; i >= 0; --i)
                        {
                            if (frame.Entries[i].Name == event.Name && frame.Entries[i].Id == event.Id)
                            {
                                entry = &frame.Entries[i];
                                break;
                            }
                        }

                        if (entry)
                        {
                            entry->Duration = event.Time - entry->StartTime;

                            stack.pop_back();
                            if (stack.empty())
                            {
                                frame.Duration = event.Time - frame.TimeStamp;
                            }
                        }
                    }
                }

                if (GTraceFrames.IsFull())
                {
                    GTraceFrames.Dequeue();
                }

                GTraceFrames.Enqueue(frame);
            }

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

            GEntityBodies.clear();

            EntityComponentsContainer<TransformComponent, BodyComponent> bodyComponents(*GCurrWorld);

            for (auto && [entity, transformComp, bodyComp] : bodyComponents)
            {
                EntityBodyShape entityBodyShape;
                entityBodyShape.Transform = transformComp->Transform;
                entityBodyShape.Radius = bodyComp->Radius;
                entityBodyShape.Color = SDL_Color(255, 0, 0);
                entityBodyShape.ZCode = transformComp->ZCode;
                entityBodyShape.VelLen = bodyComp->LinearVelocity.Length();

                if (!HasAnyFlags(bodyComp->Flags, EBodyFlags::Awake))
                {
                    entityBodyShape.Color = SDL_Color(128, 0, 0);
                }

                if (bodyComp->Movement == EBodyMovement::Attached &&
                    transformComp->AttachParent != EntityId::Invalid)
                {
                    if (TransformComponent* parentTransformComp = FeatureECS::GetComponentDataPtr<TransformComponent>(world, transformComp->AttachParent))
                    {
                        entityBodyShape.Transform.Position = parentTransformComp->Transform.Position + entityBodyShape.Transform.Position.Rotate(parentTransformComp->Transform.Rotation);
                        entityBodyShape.Transform.Rotation += parentTransformComp->Transform.Rotation;
                    }
                }

                GEntityBodies.push_back(entityBodyShape);
            }
        }
    }

    if (!GCurrWorld)
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
    TArray<FeatureSharedPtr> channelFeatures = GSession->GetFeatureSet()->GetChannelRef(WorldChannels::DebugRender);
    for (const auto& feature : channelFeatures)
    {
        feature->OnDebugRender(*GCurrWorld, *GDebugState, *GDebugRenderer);
    }

    if (GDebugState->KeyDown(SDLK_X))
    {
        GDebugRenderer->DrawCircle(mouseWorldPos, 10.0f, Color::White);
    }

    if (GDebugState->KeyDown(SDLK_F))
    {
        GDebugRenderer->DrawCircle(mouseWorldPos, 64.0f, Color::White);
    }
}

void RenderPropertyEditor(void* obj, const PropertyDescriptor& propertyDesc)
{
#define NUMERIC_EDITOR(type) \
    { \
        type v_min = std::numeric_limits<type>::min(), v_max = std::numeric_limits<type>::max(); \
        ImGui::SetNextItemWidth(-FLT_MIN); \
        type v; \
        propertyDesc.PropertyAccessor->Get(obj, &v, sizeof(type)); \
        if (ImGui::DragScalar("##Editor", ImGuiDataType_S8, &v, 1.0f, &v_min, &v_max)) \
        { \
            propertyDesc.PropertyAccessor->Set(obj, &v, sizeof(type)); \
        } \
    }

    switch (propertyDesc.ValueType)
    {
        case EPropertyValueType::Unknown:
            break;
        case EPropertyValueType::Int8:      NUMERIC_EDITOR(int8) break;
        case EPropertyValueType::UInt8:     NUMERIC_EDITOR(uint8) break;
        case EPropertyValueType::Int16:     NUMERIC_EDITOR(int16) break;
        case EPropertyValueType::UInt16:    NUMERIC_EDITOR(uint16) break;
        case EPropertyValueType::Int32:     NUMERIC_EDITOR(int32) break;
        case EPropertyValueType::UInt32:    NUMERIC_EDITOR(uint32) break;
        case EPropertyValueType::Int64:     NUMERIC_EDITOR(int64) break;
        case EPropertyValueType::UInt64:    NUMERIC_EDITOR(uint64) break;
        case EPropertyValueType::Bool:
            {
                bool v;
                propertyDesc.PropertyAccessor->Get(obj, &v, sizeof(bool));
                if (ImGui::Checkbox("##Editor", &v))
                {
                    propertyDesc.PropertyAccessor->Set(obj, &v, sizeof(bool));
                }
                break;
            }
        case EPropertyValueType::String:        break;
        case EPropertyValueType::Name:          break;
        case EPropertyValueType::FixedPoint:    break;
        default: break;
    }

#undef NUMERIC_EDITOR
}

void PlotTrace()
{

    if (!GCurrWorld)
    {
        
    }
    
    
}



struct ImGuiPlotArrayGetterData2
{
    const float* Values;
    int Stride;

    ImGuiPlotArrayGetterData2(const float* values, int stride) { Values = values; Stride = stride; }
};
static float Plot_ArrayGetter(void* data, int idx)
{
    ImGuiPlotArrayGetterData2* plot_data = (ImGuiPlotArrayGetterData2*)data;
    const float v = *(const float*)(const void*)((const unsigned char*)plot_data->Values + (size_t)idx * plot_data->Stride);
    return v;
}
void OnAppRenderUI()
{
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::Begin("Debug"))
    {
        ImGui::Text("Sim FPS:"); ImGui::SameLine(80); ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / GSessionFPS, GSessionFPS);
        ImGui::Text("SDL FPS:"); ImGui::SameLine(80); ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / GRendererFPS, GRendererFPS);
        ImGui::Text("ImGui FPS:"); ImGui::SameLine(80); ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        if (ImGui::CollapsingHeader("Features"))
        {
            for (const auto& feature : GSession->GetFeatureSet()->GetFeatures())
            {
                const auto& featureDefinition = feature->GetFeatureDefinition();
                if (featureDefinition.DisplayName.empty())
                    continue;

                if (ImGui::CollapsingHeader(featureDefinition.DisplayName.c_str()))
                {
                    if (ImGui::BeginTable("##properties", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
                    {
                        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 2.0f); // Default twice larger

                        for (const auto& methodDesc : featureDefinition.Methods | std::views::values)
                        {
                            ImGui::TableNextRow();
                            ImGui::PushID(methodDesc.Name.c_str());
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            ImGui::TextUnformatted(methodDesc.Name.c_str());
                            ImGui::TableNextColumn();

                            void* obj = methodDesc.MethodPointer->IsStatic() ? nullptr : feature.get();
                            
                            ImGui::BeginDisabled(!methodDesc.MethodPointer->CanExecute(obj));
                            if (ImGui::Button(methodDesc.Name.c_str()))
                            {
                                methodDesc.MethodPointer->Execute(obj);
                            }
                            ImGui::EndDisabled();

                            ImGui::PopID();
                        }

                        for (const auto& propertyDesc : featureDefinition.Properties | std::views::values)
                        {
                            ImGui::TableNextRow();
                            ImGui::PushID(propertyDesc.Name.c_str());
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            ImGui::TextUnformatted(propertyDesc.Name.c_str());
                            ImGui::TableNextColumn();

                            void* obj = propertyDesc.PropertyAccessor->IsStatic() ? nullptr : feature.get();
                            RenderPropertyEditor(obj, propertyDesc);

                            ImGui::PopID();
                        }

                        ImGui::EndTable();
                    }
                }
            }
        }

        ImGui::End();
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

void OnAppEvent(SDL_Event* event)
{
    int32 windowWidth, windowHeight;
    SDL_GetWindowSizeInPixels(GWindow, &windowWidth, &windowHeight);

    float mx, my;
    SDL_GetMouseState(&mx, &my);

    SDL_FPoint mouseWindowPos = { mx, my };

    Vec2 mouseWorldPos = GViewport->ViewportPosToWorldPos(mouseWindowPos);

    GDebugState->ProcessAppEvent(event);

    auto onMouseDownOrMoved = [&](const SDL_FPoint& mousePos)
    {
        // Spawn entities
        if (GDebugState->MouseDown(SDL_BUTTON_LEFT))
        {
            Action action;
            action.Verb = "spawn_entity"_n;
            action.Data[0].Name = "Unit"_n;
            action.Data[1].Distance = mouseWorldPos.X;
            action.Data[2].Distance = mouseWorldPos.Y;
            action.Data[3].Degrees = Vec2::RandUnitVector().AsRadians();
            action.Data[4].UInt32 = 100;
            GSession->QueueAction(action);
        }

        // Spawn moving entities
        if (GDebugState->KeyDown(SDLK_S))
        {
            Action action;
            action.Verb = "spawn_entity"_n;
            action.Data[0].Name = "Unit"_n;
            action.Data[1].Distance = mouseWorldPos.X;
            action.Data[2].Distance = mouseWorldPos.Y;
            action.Data[3].Degrees = Vec2::RandUnitVector().AsRadians();
            action.Data[4].UInt32 = 1;
            action.Data[5].Speed = 10;
            GSession->QueueAction(action);
        }

        // Push entities
        if (GDebugState->KeyDown(SDLK_F))
        {
            Action action;
            action.Verb = "push_entities_in_range"_n;
            action.Data[0].Distance = mouseWorldPos.X;
            action.Data[1].Distance = mouseWorldPos.Y;
            action.Data[2].Distance = 64.0f;
            action.Data[3].Value = 1000.0f;
            GSession->QueueAction(action);
        }

        // Release entities
        if (GDebugState->KeyDown(SDLK_X))
        {
            Action action;
            action.Verb = "release_entities_in_range"_n;
            action.Data[0].Distance = mouseWorldPos.X;
            action.Data[1].Distance = mouseWorldPos.Y;
            action.Data[2].Distance = 10.0f;
            GSession->QueueAction(action);
        }

        if (GCameraDragPos.IsSet())
        {
            Vec2 lastMouseWorldPos = GViewport->ViewportPosToWorldPos(*GCameraDragPos);
            Vec2 mouseDelta = mouseWorldPos - lastMouseWorldPos;
            GCamera->Position -= mouseDelta;
            GCameraDragPos = mousePos;
        }
    };

    if (event->type == SDL_EVENT_KEY_DOWN)
    {
        onMouseDownOrMoved(mouseWindowPos);

        const float CameraSpeed = 100.0f;

        if (event->key.key == SDLK_LEFT)
        {
            GCamera->Position.X -= CameraSpeed;
        }

        if (event->key.key == SDLK_RIGHT)
        {
            GCamera->Position.X += CameraSpeed;
        }

        if (event->key.key == SDLK_UP)
        {
            GCamera->Position.Y += CameraSpeed;
        }

        if (event->key.key == SDLK_DOWN)
        {
            GCamera->Position.Y -= CameraSpeed;
        }
    }
    
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (event->button.button == SDL_BUTTON_RIGHT)
        {
            GCameraDragPos = mouseWindowPos;
        }

        onMouseDownOrMoved(mouseWindowPos);
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        if (event->button.button == SDL_BUTTON_RIGHT)
        {
            GCameraDragPos.Reset();
        }
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        onMouseDownOrMoved(mouseWindowPos);
    }

    if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        float zoomScale = 1.0f + (float)event->wheel.integer_y * 0.1f;
        GViewport->Camera->Zoom = Max(GViewport->Camera->Zoom * zoomScale, 0.001);
    }
}

void OnAppShutdown()
{
    GSessionThreadWantsExit = true;
    GSessionThread->join();
}