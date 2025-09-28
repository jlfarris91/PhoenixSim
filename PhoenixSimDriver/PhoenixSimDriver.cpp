
#define NOMINMAX

#include <ctime>
#include <windows.h>

#include "FeatureECS.h"
#include "Session.h"
#include "Name.h"
#include "FeaturePhysics.h"
#include "MortonCode.h"
#include "FeatureTrace.h"
#include "Flags.h"

#define SDL_MAIN_USE_CALLBACKS
#include <queue>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>


using namespace Phoenix;
using namespace Phoenix::ECS;
using namespace Phoenix::Physics;

// the vertex input layout
struct Vertex
{
    float x, y, z;      //vec3 position
    float r, g, b, a;   //vec4 color
};

// a list of vertices
static Vertex vertices[]
{
    {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},     // top vertex
    {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f},   // bottom left vertex
    {0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}     // bottom right vertex
};

SDL_Window* GWindow = nullptr;
SDL_Renderer *GRenderer = nullptr;
int32 GWindowWidth = 800;
int32 GWindowHeight = 600;

uint32 GRendererFPSCounter = 0;
uint64 GRendererFPSTimer = 0;
float GRendererFPS = 0.0f;

Session* GSession;
bool GSessionThreadWantsExit = false;
std::thread* GSessionThread = nullptr;
std::vector<World> GWorldUpdateQueue;
std::mutex GWorldUpdateQueueMutex;
float GSessionFPS = 0;

World* GCurrWorld = nullptr;
FeaturePhysicsScratchBlock* GPhysicsScratchBlock = nullptr;

TMap<uint8, bool> GMouseButtonStates;
TMap<SDL_Keycode, bool> GKeyStates;

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
    TSharedPtr<FeatureTrace> traceFeature = std::make_shared<FeatureTrace>();
    TSharedPtr<FeatureECS> ecsFeature = std::make_shared<FeatureECS>();
    TSharedPtr<FeaturePhysics> physicsFeature = std::make_shared<FeaturePhysics>();
    
    SessionCtorArgs sessionArgs;
    sessionArgs.FeatureSetArgs.Features.push_back(traceFeature);
    sessionArgs.FeatureSetArgs.Features.push_back(ecsFeature);
    sessionArgs.FeatureSetArgs.Features.push_back(physicsFeature);
    sessionArgs.OnPostWorldUpdate = OnPostWorldUpdate;

    GSession = new Session(sessionArgs);

    GSession->Initialize();

    WorldManager* worldManager = GSession->GetWorldManager();

    auto primaryWorld = worldManager->NewWorld("TestWorld"_n);

    // int32 n = 10;
    // int32 sp = 1 << MortonCodeGridBits;
    // for (int32 x = 0; x < n; ++x)
    // {
    //     for (int32 y = 0; y < n; ++y)
    //     {
    //         ECS::EntityId entityId = ecs->AcquireEntity(*primaryWorld, "Unit"_n);
    //         ECS::BodyComponent* bodyComponent = ecs->AddComponent<ECS::BodyComponent>(*primaryWorld, entityId);
    //         bodyComponent->CollisionMask = 1;
    //         bodyComponent->Radius = 12.0f;
    //         bodyComponent->Transform.Position.X = sp + sp * 0.5 + x * sp;
    //         bodyComponent->Transform.Position.Y = sp + sp * 0.5 + y * sp;
    //         // bodyComponent->Transform.Rotation = Vec2::RandUnitVector().AsDegrees();
    //         // bodyComponent->Mass = (rand() % 1000) / 1000.0f * 1.0f;
    //     }
    // }

    for (int32 i = 0; i < 00; ++i)
    {
        EntityId entityId = ecsFeature->AcquireEntity(*primaryWorld, "Unit"_n);

        TransformComponent* transformComp = ecsFeature->AddComponent<TransformComponent>(*primaryWorld, entityId);
        transformComp->Transform.Position.X = 2000;
        transformComp->Transform.Position.Y = 1000;
        transformComp->Transform.Rotation = Vec2::RandUnitVector().AsDegrees();
        
        BodyComponent* bodyComp = ecsFeature->AddComponent<BodyComponent>(*primaryWorld, entityId);
        bodyComp->CollisionMask = 1;
        bodyComp->Radius = 16.0f;
        // bodyComponent->Mass = (rand() % 1000) / 1000.0f * 1.0f;

        // ECS::EntityId entityId2 = ecs->AcquireEntity(*primaryWorld, "Unit"_n);
        // ECS::BodyComponent* bodyComponent2 = ecs->AddComponent<ECS::BodyComponent>(*primaryWorld, entityId2);
        // bodyComponent2->AttachParent = entityId;
        // bodyComponent2->Movement = ECS::EBodyMovement::Attached;
        // bodyComponent2->Radius = 6.0f;
        // bodyComponent2->Transform.Position = Vec2::XAxis * -bodyComponent->Radius;
        // bodyComponent2->Transform.Rotation = bodyComponent->Transform.Rotation + 180.0f;
    }

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

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    InitSession();


    auto mag0 = Vec2(1, 1).Length();
    auto mag1 = Vec2(10, 10).Length();
    auto mag2 = Vec2(10, 0).Length();
    auto mag3 = Vec2(0, 10).Length();

    auto rot0 = Cordic::Rotate<Distance>(10, 10, HALF_PI);
    auto rot1 = Cordic::Rotate<Distance>(0, 10, PI);

    auto sqrt0 = Sqrt(Value(1.0));
    auto sqrt1 = Sqrt(Value(4.0));
    auto sqrt2 = Sqrt(Value(16.0));
    auto sqrt3 = Sqrt(Value(8.0*8.0));
    auto sqrt4 = Sqrt(Value(16.0*16.0));
    auto sqrt5 = Sqrt(Value(32.0*32.0));
    auto sqrt6 = Sqrt(Value(64.0*64.0));
    auto sqrt7 = Sqrt(Value(128.0*128.0));
    
    auto cos0 = Cos(Deg2Rad(-360));
    auto cos1 = Cos(Deg2Rad(-315));
    auto cos2 = Cos(Deg2Rad(-270));
    auto cos3 = Cos(Deg2Rad(-225));
    auto cos4 = Cos(Deg2Rad(-180));
    auto cos5 = Cos(Deg2Rad(-135));
    auto cos6 = Cos(Deg2Rad(-90));
    auto cos7 = Cos(Deg2Rad(-45));
    auto cos8 = Cos(Deg2Rad(0));
    auto cos9 = Cos(Deg2Rad(45));
    auto cos10 = Cos(Deg2Rad(90));
    auto cos11 = Cos(Deg2Rad(135));
    auto cos12 = Cos(Deg2Rad(180));
    auto cos13 = Cos(Deg2Rad(225));
    auto cos14 = Cos(Deg2Rad(270));
    auto cos15 = Cos(Deg2Rad(315));
    auto cos16 = Cos(Deg2Rad(360));

    auto sin0 = Sin(Deg2Rad(-360));
    auto sin1 = Sin(Deg2Rad(-315));
    auto sin2 = Sin(Deg2Rad(-270));
    auto sin3 = Sin(Deg2Rad(-225));
    auto sin4 = Sin(Deg2Rad(-180));
    auto sin5 = Sin(Deg2Rad(-135));
    auto sin6 = Sin(Deg2Rad(-90));
    auto sin7 = Sin(Deg2Rad(-45));
    auto sin8 = Sin(Deg2Rad(0));
    auto sin9 = Sin(Deg2Rad(45));
    auto sin10 = Sin(Deg2Rad(90));
    auto sin11 = Sin(Deg2Rad(135));
    auto sin12 = Sin(Deg2Rad(180));
    auto sin13 = Sin(Deg2Rad(225));
    auto sin14 = Sin(Deg2Rad(270));
    auto sin15 = Sin(Deg2Rad(315));
    auto sin16 = Sin(Deg2Rad(360));

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    GRendererFPSTimer = SDL_GetTicks();

    if (!SDL_CreateWindowAndRenderer("Phoenix", GWindowWidth, GWindowHeight, SDL_WINDOW_RESIZABLE, &GWindow, &GRenderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GetWindowSizeInPixels(GWindow, &GWindowWidth, &GWindowHeight);
    Action action;
    action.Verb = "set_map_center"_n;
    action.Data[0].Distance = GWindowWidth >> 1;
    action.Data[1].Distance = GWindowHeight >> 1;
    GSession->QueueAction(action);

    return SDL_APP_CONTINUE;
}

void SDL_RenderCircle(SDL_Renderer *renderer, float x1, float y1, float radius, int32 segments = 32)
{
    std::vector<SDL_FPoint> points;
    points.reserve(segments);

    for (int32 i = 0; i < segments - 1; ++i)
    {
        float angle = float(i) * (2.0f * 3.14f) / (segments - 1);
        float x = x1 + cos(angle) * radius; 
        float y = y1 + sin(angle) * radius;
        points.emplace_back(x, y);
    }

    points.push_back(points[0]);
    
    SDL_RenderLines(renderer, points.data(), segments);
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
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

            GPhysicsScratchBlock = GCurrWorld->GetBlock<FeaturePhysicsScratchBlock>();

            GEntityBodies.clear();

            EntityComponentsContainer<TransformComponent, BodyComponent> bodyComponents(world);

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

            GWorldUpdateQueue.clear();
        }
    }

    if (!GCurrWorld)
        return SDL_APP_CONTINUE;

    float mx, my;
    SDL_GetMouseState(&mx, &my);

    SDL_GetWindowSizeInPixels(GWindow, &GWindowWidth, &GWindowHeight);

    /* as you can see from this, rendering draws over whatever was drawn before it. */
    SDL_SetRenderDrawColor(GRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* black, full alpha */
    SDL_RenderClear(GRenderer);  /* start with a blank canvas. */

    /* Let's draw a single rectangle (square, really). */

    Vec2 mapSize(512, 512);
    Vec2 windowCenter(GWindowWidth / 2.0f, GWindowHeight / 2.0f);

    SDL_FRect mapRect;
    mapRect.x = (float)mapSize.X / 2;
    mapRect.y = (float)mapSize.Y / 2;
    mapRect.w = (float)mapSize.X;
    mapRect.h = (float)mapSize.Y;
    // SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    // SDL_RenderRect(renderer, &mapRect);

    // Vec2 mapCenter(GWindowWidth >> 1, GWindowHeight >> 1);
    Vec2 mapCenter(0, 0);

    SDL_SetRenderDrawColor(GRenderer, 30, 30, 30, SDL_ALPHA_OPAQUE);

    for (int32 i = 0; i < ceil(GWindowWidth / 10); ++i)
    {
        float x = i * (1 << MortonCodeGridBits);
        SDL_RenderLine(GRenderer, x, 0, x, GWindowHeight);
    }
    
    for (int32 i = 0; i < ceil(GWindowHeight / 10); ++i)
    {
        float y = i * (1 << MortonCodeGridBits);
        SDL_RenderLine(GRenderer, 0, y, GWindowWidth, y);
    }

    SDL_SetRenderDrawColor(GRenderer, 0, 255, 0, SDL_ALPHA_OPAQUE);

    if (GPhysicsScratchBlock)
    {
        for (const CollisionLine& collisionLine : GPhysicsScratchBlock->CollisionLines)
        {
            SDL_RenderLine(GRenderer,
                (float)mapCenter.X + (float)collisionLine.Line.Start.X,
                (float)mapCenter.Y + (float)collisionLine.Line.Start.Y,
                (float)mapCenter.X + (float)collisionLine.Line.End.X,
                (float)mapCenter.Y + (float)collisionLine.Line.End.Y);

            // Vec2 v = Line2::VectorToLine(collisionLine.Line, Vec2(mx, my));
            // int32 vX = (int32)v.X;
            // int32 vY = (int32)v.Y;
            // SDL_RenderLine(GRenderer, mx, my, mx + vX, my + vY);
        }

        // for (const Contact& contact : GPhysicsScratchBlock->Contacts)
        // {
        //     Vec2 v = contact.Normal * contact.Bias;
        //     Vec2 s = contact.TransformA->Transform.Position;
        //     Vec2 e = s + v;
        //     SDL_RenderLine(GRenderer, s.X, s.Y, e.X, e.Y);
        // }
    }

    static TArray<TTuple<uint32, uint32, uint32>> queryCodeColors;
    if (queryCodeColors.empty())
    {
    }

    for (const EntityBodyShape& entityBodyShape : GEntityBodies)
    {
        SDL_SetRenderDrawColor(GRenderer, entityBodyShape.Color.r, entityBodyShape.Color.g, entityBodyShape.Color.b, SDL_ALPHA_OPAQUE);

        Vec2 pt1 = Vec2::XAxis * entityBodyShape.Radius;
        Vec2 pt2 = pt1.Rotate(Deg2Rad(-135));
        Vec2 pt3 = pt1.Rotate(Deg2Rad(135));

        Transform2D transform(Vec2::Zero, entityBodyShape.Transform.Rotation, 1.0f);
        pt1 = mapCenter + entityBodyShape.Transform.Position + transform.RotateVector(pt1);
        pt2 = mapCenter + entityBodyShape.Transform.Position + transform.RotateVector(pt2);
        pt3 = mapCenter + entityBodyShape.Transform.Position + transform.RotateVector(pt3);

        SDL_FPoint points[4];
        points[0] = { (float)pt1.X, (float)pt1.Y };
        points[1] = { (float)pt2.X, (float)pt2.Y };
        points[2] = { (float)pt3.X, (float)pt3.Y };
        points[3] = { (float)pt1.X, (float)pt1.Y };
        SDL_RenderLines(GRenderer, points, 4);
    }

    // Render some debug text for each body
    if (0)
    {
        constexpr float scale = 2.0f;
        SDL_SetRenderDrawColor(GRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_SetRenderScale(GRenderer, scale, scale);
    
        for (const EntityBodyShape& entityBodyShape : GEntityBodies)
        {
            Vec2 pt1 = Vec2::XAxis * entityBodyShape.Radius;
            Transform2D transform(Vec2::Zero, entityBodyShape.Transform.Rotation, 1.0f);
            pt1 = mapCenter + entityBodyShape.Transform.Position + transform.RotateVector(pt1);
    
            char zcodeStr[256] = { '\0' };
            sprintf_s(zcodeStr, _countof(zcodeStr), "%llu", entityBodyShape.ZCode);
            SDL_RenderDebugText(GRenderer, (float)pt1.X / scale, (float)pt1.Y / scale, zcodeStr);
        }
    
        SDL_SetRenderScale(GRenderer, 1.0f, 1.0f);
    }
    
    /* top right quarter of the window. */
    // SDL_Rect viewport;
    // viewport.x = 0;
    // viewport.y = 0;
    // viewport.w = windowWidth;
    // viewport.h = windowHeight;
    // SDL_SetRenderViewport(renderer, &viewport);
    // SDL_SetRenderClipRect(renderer, &viewport);

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

    float textY = 10;

#define RenderDebugText(format, ...) \
    { \
        char __str[256] = { '\0' }; \
        sprintf_s(__str, _countof(__str), format, __VA_ARGS__); \
        SDL_RenderDebugText(GRenderer, 10, textY, __str); \
        textY += 10; \
    }

    SDL_SetRenderDrawColor(GRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);  /* white, full alpha */
    SDL_SetRenderScale(GRenderer, 1.5f, 1.5f);

    RenderDebugText("FPS: %.2f", GRendererFPS)
    RenderDebugText("Sim: %.2f", GSessionFPS)
    RenderDebugText("Bodies: %llu", GEntityBodies.size())

    RenderDebugText("%.0f, %.0f", mx, my)

    if (GPhysicsScratchBlock)
    {
        RenderDebugText("Num Collisions: %llu", GPhysicsScratchBlock->NumCollisions)
        RenderDebugText("Num Iterations: %llu", GPhysicsScratchBlock->NumIterations)
        RenderDebugText("Max Query Bodies: %llu", GPhysicsScratchBlock->MaxQueryBodyCount)
        RenderDebugText("Num Contacts: %llu", GPhysicsScratchBlock->Contacts.Num())
    }

    FeatureTraceScratchBlock& traceBlock = GCurrWorld->GetBlockRef<FeatureTraceScratchBlock>();

    struct AggTraceEvent
    {
        FName Name;
        FName Id;
        uint32 Count = 0;
        clock_t Total = 0;
        clock_t Max = 0;
        clock_t StartTime = 0;
    };
    
    std::vector<AggTraceEvent> traceEvents;

    RenderDebugText("Trace: %llu events", traceBlock.Events.Num())
    
    uint32 indent = 0;
    std::vector<FName> traceEventStack;
    char indentStr[32];
    for (const TraceEvent& traceEvent : traceBlock.Events)
    {
        AggTraceEvent* aggEvent = nullptr;
        for (AggTraceEvent& asdf : traceEvents)
        {
            if (asdf.Name == traceEvent.Name && asdf.Id == traceEvent.Id)
            {
                aggEvent = &asdf;
                break;
            }
        }

        if (!aggEvent)
        {
            traceEvents.push_back({ traceEvent.Name, traceEvent.Id});
            aggEvent = &traceEvents.back();
        }
        
        if (traceEvent.Flag == ETraceFlags::Begin)
        {
            aggEvent->Count++;
            aggEvent->StartTime = traceEvent.Time;
            // traceEventStack.push_back(traceEvent.Name);
        }

        if (traceEvent.Flag == ETraceFlags::End)
        {
            aggEvent->Count += traceEvent.Counter;
            aggEvent->Total += traceEvent.Time - aggEvent->StartTime;
            aggEvent->Max = std::max(aggEvent->Total, aggEvent->Max);
            // traceEventStack.pop_back();
        }

        if (traceEvent.Flag == ETraceFlags::Counter)
        {
            aggEvent->Count += traceEvent.Counter;            
        }
    }

    if (0)
    {
        for (const AggTraceEvent& traceEvent : traceEvents)
        {
    #if DEBUG
            RenderDebugText("%s %s %u %.3f %.3f",
                traceEvent.Name.Debug,
                traceEvent.Id.Debug,
                traceEvent.Count,
                (float)traceEvent.Total / CLOCKS_PER_SEC,
                (float)traceEvent.Max / CLOCKS_PER_SEC)
    #endif
        }
    }
    
    // {
    //          uint32 mxmc = uint32(mx) >> MortonCodeGridBits;
    //          uint32 mymc = uint32(my) >> MortonCodeGridBits;
    //          uint64 mmc = MortonCode(mxmc, mymc);
    //  
    //          RenderDebugText("MC: %llu", mmc)
    //  
    //          // Query for overlapping morton ranges
    //          static TArray<TTuple<uint64, uint64>> ranges;
    //          {
    //              constexpr float mouseRadius = 12.0f;
    //              uint32 lox = static_cast<uint32>(mx - mouseRadius);
    //              uint32 hix = static_cast<uint32>(mx + mouseRadius);
    //              uint32 loy = static_cast<uint32>(my - mouseRadius);                
    //              uint32 hiy = static_cast<uint32>(my + mouseRadius);
    //  
    //              SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    //  
    //              SDL_FRect mouseRect(mx - mouseRadius, my - mouseRadius, mouseRadius * 2, mouseRadius * 2);
    //              SDL_RenderRect(renderer, &mouseRect);
    //  
    //              SDL_SetRenderScale(renderer, 2.0f, 2.0f);
    //  
    //              MortonCodeAABB aabb;
    //              aabb.MinX = lox >> MortonCodeGridBits;
    //              aabb.MinY = loy >> MortonCodeGridBits;
    //              aabb.MaxX = hix >> MortonCodeGridBits;
    //              aabb.MaxY = hiy >> MortonCodeGridBits;
    //  
    //              RenderDebugText("(%llu, %llu, %llu, %llu)", aabb.MinX, aabb.MinY, aabb.MaxX, aabb.MaxY)
    //  
    //              uint64 bl = MortonCode(lox >> MortonCodeGridBits, loy >> MortonCodeGridBits);
    //              uint64 br = MortonCode(hix >> MortonCodeGridBits, loy >> MortonCodeGridBits);
    //              uint64 tl = MortonCode(lox >> MortonCodeGridBits, hiy >> MortonCodeGridBits);
    //              uint64 tr = MortonCode(hix >> MortonCodeGridBits, hiy >> MortonCodeGridBits);
    //              
    //              RenderDebugText("(%llu, %llu, %llu, %llu)", bl, br, tl, tr)
    //  
    //              char mortonCodeAABBStr[256];
    //              sprintf_s(mortonCodeAABBStr, _countof(mortonCodeAABBStr), "%llu", bl);
    //              SDL_RenderDebugText(renderer, lox * 0.5, loy * 0.5, mortonCodeAABBStr);
    //              sprintf_s(mortonCodeAABBStr, _countof(mortonCodeAABBStr), "%llu", br);
    //              SDL_RenderDebugText(renderer, hix * 0.5, loy * 0.5, mortonCodeAABBStr);
    //              sprintf_s(mortonCodeAABBStr, _countof(mortonCodeAABBStr), "%llu", tl);
    //              SDL_RenderDebugText(renderer, lox * 0.5, hiy * 0.5, mortonCodeAABBStr);
    //              sprintf_s(mortonCodeAABBStr, _countof(mortonCodeAABBStr), "%llu", tr);
    //              SDL_RenderDebugText(renderer, hix * 0.5, hiy * 0.5, mortonCodeAABBStr);
    //              
    //              ranges.clear();
    //              MortonCodeQuery(aabb, ranges);
    //          }
    //  
    //          int y = 80;
    //          for (auto && [min, max] : ranges)
    //          {
    //              char mortonCodeRangeStr[256] = { '\0' };
    //              sprintf_s(mortonCodeRangeStr, _countof(mortonCodeRangeStr), "  > %llu, %llu", min, max);
    //              SDL_RenderDebugText(renderer, 10, y, mortonCodeRangeStr);
    //              y += 10;
    //          }
    //      }

    {
        Distance cx = (float)windowCenter.X;
        Distance cy = (float)windowCenter.Y;
        Distance fmx = mx;
        Distance fmy = GWindowHeight - my;
        auto dx = fmx - cx;
        auto dy = fmy - cy;

        Angle a = Cordic::ArcTan2<Distance>(dy, dx);
        Distance m = Magnitude(dx, dy);

        RenderDebugText("dx: %f, dy: %f, m: %f, a: %f", (float)dx, (float)dy, (float)m, (float)a)

        Vec2 v(1, 0);
        v = v.Rotate(a);

        float x1 = (float)cx;
        float y1 = (float)cy;
        float x2 = (float)v.X * (float)m;
        float y2 = -(float)v.Y * (float)m;

        SDL_SetRenderScale(GRenderer, 1.0f, 1.0f);
        SDL_RenderLine(GRenderer, x1, y1, x1 + x2, y1 + y2);
    }

    SDL_SetRenderScale(GRenderer, 1.0f, 1.0f);

    if (GMouseButtonStates.contains(SDL_BUTTON_RIGHT) && GMouseButtonStates[SDL_BUTTON_RIGHT])
    {
        SDL_RenderCircle(GRenderer, mx, my, 64.0f);
    }

    if (GKeyStates.contains(SDLK_X) && GKeyStates[SDLK_X])
    {
        SDL_RenderCircle(GRenderer, mx, my, 64.0f);
    }

    if (GKeyStates.contains(SDLK_F) && GKeyStates[SDLK_F])
    {
        SDL_RenderCircle(GRenderer, mx, my, 64.0f);
    }

    SDL_RenderPresent(GRenderer);  /* put it all on the screen! */

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // close the window on request
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
    }

    auto mapCenterX = 0; //GWindowWidth >> 1;
    auto mapCenterY = 0; //GWindowHeight >> 1;

    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
        SDL_GetWindowSizeInPixels(GWindow, &GWindowWidth, &GWindowHeight);
        Action action;
        action.Verb = "set_map_center"_n;
        action.Data[0].Distance = GWindowWidth >> 1;
        action.Data[1].Distance = GWindowHeight >> 1;
        GSession->QueueAction(action);
    }

    auto onMouseDownOrMoved = [&](const SDL_FPoint& mousePos)
    {
        // Spawn entities
        if (GMouseButtonStates.contains(SDL_BUTTON_LEFT) && GMouseButtonStates[SDL_BUTTON_LEFT])
        {
            Action action;
            action.Verb = "spawn_entity"_n;
            action.Data[0].Name = "Unit"_n;
            action.Data[1].Distance = mousePos.x - mapCenterX;
            action.Data[2].Distance = mousePos.y - mapCenterY;
            action.Data[3].Degrees = Vec2::RandUnitVector().AsRadians();
            action.Data[4].UInt32 = 1;
            GSession->QueueAction(action);
        }

        // Spawn moving entities
        if (GKeyStates.contains(SDLK_S) && GKeyStates[SDLK_S])
        {
            Action action;
            action.Verb = "spawn_entity"_n;
            action.Data[0].Name = "Unit"_n;
            action.Data[1].Distance = mousePos.x - mapCenterX;
            action.Data[2].Distance = mousePos.y - mapCenterY;
            action.Data[3].Degrees = Vec2::RandUnitVector().AsRadians();
            action.Data[4].UInt32 = 1;
            action.Data[5].Speed = 10;
            GSession->QueueAction(action);
        }
        
        // Push entities
        if (GKeyStates.contains(SDLK_F) && GKeyStates[SDLK_F])
        {
            Action action;
            action.Verb = "push_entities_in_range"_n;
            action.Data[0].Distance = mousePos.x - mapCenterX;
            action.Data[1].Distance = mousePos.y - mapCenterY;
            action.Data[2].Distance = 64.0f;
            action.Data[3].Value = 10000.0f;
            GSession->QueueAction(action);
        }

        // Release entities
        if (GKeyStates.contains(SDLK_X) && GKeyStates[SDLK_X])
        {
            Action action;
            action.Verb = "release_entities_in_range"_n;
            action.Data[0].Distance = mousePos.x - mapCenterX;
            action.Data[1].Distance = mousePos.y - mapCenterY;
            action.Data[2].Distance = 64.0f;
            GSession->QueueAction(action);
        }
    };

    if (event->type == SDL_EVENT_KEY_DOWN)
    {
        GKeyStates[event->key.key] = true;

        SDL_FPoint mousePos;
        SDL_GetMouseState(&mousePos.x, &mousePos.y);
        onMouseDownOrMoved(mousePos);
    }

    if (event->type == SDL_EVENT_KEY_UP)
    {
        GKeyStates[event->key.key] = false;
    }
    
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        GMouseButtonStates[event->button.button] = true;

        SDL_FPoint mousePos = { event->button.x, event->button.y };
        onMouseDownOrMoved(mousePos);
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        GMouseButtonStates[event->button.button] = false;
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        SDL_FPoint mousePos = { event->button.x, event->button.y };
        onMouseDownOrMoved(mousePos);
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    GSessionThreadWantsExit = true;
    GSessionThread->join();

    // destroy the window
    SDL_DestroyWindow(GWindow);
}