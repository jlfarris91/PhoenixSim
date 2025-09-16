
#include <ctime>
#include <windows.h>

#include "../PhoenixSim/Public/FeatureECS.h"
#include "../PhoenixSim/Public/Session.h"
#include "../PhoenixSim/Public/Name.h"
#include "../PhoenixSim/Public/FeaturePhysics.h"
#include "../PhoenixSim/Public/MortonCode.h"

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

SDL_Window* window = nullptr;
SDL_Renderer *renderer = nullptr;
int32 windowWidth = 800;
int32 windowHeight = 600;

uint32 rendererFPSCounter = 0;
uint64 rendererFPSTimer = 0;
float rendererFPS = 0.0f;

Session* session;
bool sessionThreadWantsExit = false;
std::thread* sessionThread = nullptr;
std::vector<World> worldUpdateQueue;
std::mutex worldUpdateQueueMutex;

float sessionFPS = 0;

struct EntityBodyShape
{
    Transform2D Transform;
    Distance Radius;
    uint64 ZCode;
};

std::vector<EntityBodyShape> entityBodies;

void UpdateSessionWorker();
void OnPostWorldUpdate(WorldConstRef world);

void InitSession()
{
    TSharedPtr<FeatureECS> ecsFeature = std::make_shared<FeatureECS>();
    TSharedPtr<FeaturePhysics> physicsFeature = std::make_shared<FeaturePhysics>();
    
    SessionCtorArgs sessionArgs;
    sessionArgs.FeatureSetArgs.Features.push_back(ecsFeature);
    sessionArgs.FeatureSetArgs.Features.push_back(physicsFeature);
    sessionArgs.OnPostWorldUpdate = OnPostWorldUpdate;

    session = new Session(sessionArgs);

    session->Initialize();

    WorldManager* worldManager = session->GetWorldManager();

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
        BodyComponent* bodyComponent = ecsFeature->AddComponent<BodyComponent>(*primaryWorld, entityId);
        bodyComponent->CollisionMask = 1;
        bodyComponent->Radius = 16.0f;
        bodyComponent->Transform.Position.X = 2000;
        bodyComponent->Transform.Position.Y = 1000;
        bodyComponent->Transform.Rotation = Vec2::RandUnitVector().AsDegrees();
        // bodyComponent->Mass = (rand() % 1000) / 1000.0f * 1.0f;

        // ECS::EntityId entityId2 = ecs->AcquireEntity(*primaryWorld, "Unit"_n);
        // ECS::BodyComponent* bodyComponent2 = ecs->AddComponent<ECS::BodyComponent>(*primaryWorld, entityId2);
        // bodyComponent2->AttachParent = entityId;
        // bodyComponent2->Movement = ECS::EBodyMovement::Attached;
        // bodyComponent2->Radius = 6.0f;
        // bodyComponent2->Transform.Position = Vec2::XAxis * -bodyComponent->Radius;
        // bodyComponent2->Transform.Rotation = bodyComponent->Transform.Rotation + 180.0f;
    }

    sessionThread = new std::thread(UpdateSessionWorker);
}

void UpdateSessionWorker()
{
    clock_t lastClockTime = 0;
    sessionThreadWantsExit = false;

    uint32 sessionFPSCounter = 0;
    clock_t sessionFPSTimer = clock();

    while (!sessionThreadWantsExit)
    {
        clock_t currClockTime = clock();
        clock_t deltaClockTime = currClockTime - lastClockTime;
        lastClockTime = currClockTime;
        float deltaTime = static_cast<float>(static_cast<double>(deltaClockTime) / CLOCKS_PER_SEC);

        ++sessionFPSCounter;
        clock_t sessionFPSTimerDelta = currClockTime - sessionFPSTimer;
        if (sessionFPSTimerDelta > 1 * CLOCKS_PER_SEC)
        {
            sessionFPSTimer = currClockTime;
            sessionFPS = static_cast<float>(sessionFPSCounter);
            sessionFPS += static_cast<float>(sessionFPSTimerDelta) / CLOCKS_PER_SEC;
            sessionFPSCounter = 0;
        }
        
        SessionStepArgs stepArgs;
        stepArgs.DeltaTime = deltaTime;
            
        session->Step(stepArgs);
        Sleep(10);
    }
}

void OnPostWorldUpdate(WorldConstRef world)
{
    std::lock_guard lock(worldUpdateQueueMutex);
    worldUpdateQueue.push_back(world);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    InitSession();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    rendererFPSTimer = SDL_GetTicks();

    if (!SDL_CreateWindowAndRenderer("Phoenix", windowWidth, windowHeight, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    {
        std::lock_guard lock(worldUpdateQueueMutex);

        for (size_t i = 0; i < worldUpdateQueue.size(); ++i)
        {
            World& world = worldUpdateQueue[i];

            entityBodies.clear();

            ECSComponentAccessor<BodyComponent> componentsAccessor(world);

            for (auto && [entity, body] : componentsAccessor)
            {
                EntityBodyShape entityBodyShape;
                entityBodyShape.Transform = body->Transform;
                entityBodyShape.Radius = body->Radius;
                entityBodyShape.ZCode = body->ZCode;

                if (body->Movement == EBodyMovement::Attached &&
                    body->AttachParent != EntityId::Invalid)
                {
                    if (BodyComponent* parentBody = FeatureECS::GetComponentDataPtr<BodyComponent>(world, body->AttachParent))
                    {
                        entityBodyShape.Transform.Position = parentBody->Transform.Position + entityBodyShape.Transform.Position.Rotate(parentBody->Transform.Rotation);
                        entityBodyShape.Transform.Rotation += parentBody->Transform.Rotation;
                    }
                }

                entityBodies.push_back(entityBodyShape);
            }
        }

        worldUpdateQueue.clear();
    }

    SDL_GetWindowSizeInPixels(window, &windowWidth, &windowHeight);

    /* as you can see from this, rendering draws over whatever was drawn before it. */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* black, full alpha */
    SDL_RenderClear(renderer);  /* start with a blank canvas. */

    /* Let's draw a single rectangle (square, really). */

    Vec2 mapSize(512, 512);
    Vec2 windowCenter(windowWidth / 2.0f, windowHeight / 2.0f);

    SDL_FRect mapRect;
    mapRect.x = mapSize.X / 2;
    mapRect.y = mapSize.Y / 2;
    mapRect.w = mapSize.X;
    mapRect.h = mapSize.Y;
    // SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    // SDL_RenderRect(renderer, &mapRect);

    SDL_SetRenderDrawColor(renderer, 30, 30, 30, SDL_ALPHA_OPAQUE);

    for (int32 i = 0; i < ceil(windowWidth / 10); ++i)
    {
        float x = i * (1 << MortonCodeGridBits);
        SDL_RenderLine(renderer, x, 0, x, windowHeight);
    }
    
    for (int32 i = 0; i < ceil(windowHeight / 10); ++i)
    {
        float y = i * (1 << MortonCodeGridBits);
        SDL_RenderLine(renderer, 0, y, windowWidth, y);
    }

    static TArray<TTuple<uint32, uint32, uint32>> queryCodeColors;
    if (queryCodeColors.empty())
    {
    }

    for (const EntityBodyShape& entityBodyShape : entityBodies)
    {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);

        Vec2 pt1 = Vec2::XAxis * entityBodyShape.Radius;
        Vec2 pt2 = pt1.Rotate(-135);
        Vec2 pt3 = pt1.Rotate(135);

        Transform2D transform(Vec2::Zero, entityBodyShape.Transform.Rotation, 1.0f);
        pt1 = entityBodyShape.Transform.Position + transform.RotateVector(pt1);
        pt2 = entityBodyShape.Transform.Position + transform.RotateVector(pt2);
        pt3 = entityBodyShape.Transform.Position + transform.RotateVector(pt3);

        SDL_FPoint points[4];
        points[0] = { pt1.X, pt1.Y };
        points[1] = { pt2.X, pt2.Y };
        points[2] = { pt3.X, pt3.Y };
        points[3] = { pt1.X, pt1.Y };
        SDL_RenderLines(renderer, points, 4);
    }

    // {
    //     constexpr float scale = 2.0f;
    //     SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    //     SDL_SetRenderScale(renderer, scale, scale);
    //
    //     for (const EntityBodyShape& entityBodyShape : entityBodies)
    //     {
    //         Vec2 pt1 = Vec2::XAxis * entityBodyShape.Radius;
    //         Transform2D transform(Vec2::Zero, entityBodyShape.Transform.Rotation, 1.0f);
    //         pt1 = entityBodyShape.Transform.Position + transform.RotateVector(pt1);
    //
    //         char zcodeStr[256] = { '\0' };
    //         sprintf_s(zcodeStr, _countof(zcodeStr), "%llu", entityBodyShape.NumQueryBodies);
    //         SDL_RenderDebugText(renderer, pt1.X / scale, pt1.Y / scale, zcodeStr);
    //     }
    //
    //     SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    // }
    
    /* top right quarter of the window. */
    // SDL_Rect viewport;
    // viewport.x = 0;
    // viewport.y = 0;
    // viewport.w = windowWidth;
    // viewport.h = windowHeight;
    // SDL_SetRenderViewport(renderer, &viewport);
    // SDL_SetRenderClipRect(renderer, &viewport);

    ++rendererFPSCounter;
    Uint64 currTicks = SDL_GetTicks();
    unsigned long long tickDelta = currTicks - rendererFPSTimer;
    if (tickDelta > CLOCKS_PER_SEC)
    {
        rendererFPSTimer = currTicks;
        rendererFPS = static_cast<float>(rendererFPSCounter);
        rendererFPS += static_cast<float>(tickDelta) / CLOCKS_PER_SEC;
        rendererFPSCounter = 0;
    }

    float textY = 10;

#define RenderDebugText(format, ...) \
    { \
        char __str[256] = { '\0' }; \
        sprintf_s(__str, _countof(__str), format, __VA_ARGS__); \
        SDL_RenderDebugText(renderer, 10, textY, __str); \
        textY += 10; \
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);  /* white, full alpha */
    SDL_SetRenderScale(renderer, 2.0f, 2.0f);

    RenderDebugText("FPS: %.2f", rendererFPS)
    RenderDebugText("Sim: %.2f", sessionFPS)
    RenderDebugText("Bodies: %llu", entityBodies.size())

    float mx, my;
    SDL_GetMouseState(&mx, &my);

    RenderDebugText("%.0f, %.0f", mx, my)

    {
        uint32 mxmc = uint32(mx) >> MortonCodeGridBits;
        uint32 mymc = uint32(my) >> MortonCodeGridBits;
        uint64 mmc = MortonCode(mxmc, mymc);

        RenderDebugText("MC: %llu", mmc)

        // Query for overlapping morton ranges
        static TArray<TTuple<uint64, uint64>> ranges;
        {
            constexpr float mouseRadius = 12.0f;
            uint32 lox = static_cast<uint32>(mx - mouseRadius);
            uint32 hix = static_cast<uint32>(mx + mouseRadius);
            uint32 loy = static_cast<uint32>(my - mouseRadius);                
            uint32 hiy = static_cast<uint32>(my + mouseRadius);

            SDL_SetRenderScale(renderer, 1.0f, 1.0f);

            SDL_FRect mouseRect(mx - mouseRadius, my - mouseRadius, mouseRadius * 2, mouseRadius * 2);
            SDL_RenderRect(renderer, &mouseRect);

            SDL_SetRenderScale(renderer, 2.0f, 2.0f);

            MortonCodeAABB aabb;
            aabb.MinX = lox >> MortonCodeGridBits;
            aabb.MinY = loy >> MortonCodeGridBits;
            aabb.MaxX = hix >> MortonCodeGridBits;
            aabb.MaxY = hiy >> MortonCodeGridBits;

            RenderDebugText("(%llu, %llu, %llu, %llu)", aabb.MinX, aabb.MinY, aabb.MaxX, aabb.MaxY)

            uint64 bl = MortonCode(lox >> MortonCodeGridBits, loy >> MortonCodeGridBits);
            uint64 br = MortonCode(hix >> MortonCodeGridBits, loy >> MortonCodeGridBits);
            uint64 tl = MortonCode(lox >> MortonCodeGridBits, hiy >> MortonCodeGridBits);
            uint64 tr = MortonCode(hix >> MortonCodeGridBits, hiy >> MortonCodeGridBits);
            
            RenderDebugText("(%llu, %llu, %llu, %llu)", bl, br, tl, tr)

            char mortonCodeAABBStr[256];
            sprintf_s(mortonCodeAABBStr, _countof(mortonCodeAABBStr), "%llu", bl);
            SDL_RenderDebugText(renderer, lox * 0.5, loy * 0.5, mortonCodeAABBStr);
            sprintf_s(mortonCodeAABBStr, _countof(mortonCodeAABBStr), "%llu", br);
            SDL_RenderDebugText(renderer, hix * 0.5, loy * 0.5, mortonCodeAABBStr);
            sprintf_s(mortonCodeAABBStr, _countof(mortonCodeAABBStr), "%llu", tl);
            SDL_RenderDebugText(renderer, lox * 0.5, hiy * 0.5, mortonCodeAABBStr);
            sprintf_s(mortonCodeAABBStr, _countof(mortonCodeAABBStr), "%llu", tr);
            SDL_RenderDebugText(renderer, hix * 0.5, hiy * 0.5, mortonCodeAABBStr);
            
            ranges.clear();
            MortonCodeQuery(aabb, ranges);
        }

        int y = 80;
        for (auto && [min, max] : ranges)
        {
            char mortonCodeRangeStr[256] = { '\0' };
            sprintf_s(mortonCodeRangeStr, _countof(mortonCodeRangeStr), "  > %llu, %llu", min, max);
            SDL_RenderDebugText(renderer, 10, y, mortonCodeRangeStr);
            y += 10;
        }
    }

    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    SDL_RenderPresent(renderer);  /* put it all on the screen! */

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // close the window on request
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        for (int32 i = 0; i < 100; ++i)
        {
            Action action;
            action.Verb = "spawn_entity"_n;
            action.Data[0].Name = "Unit"_n;
            action.Data[1].Distance = event->button.x;
            action.Data[2].Distance = event->button.y;
            action.Data[3].Degrees = Vec2::RandUnitVector().AsDegrees();
            session->QueueAction(0, action);
        }
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    sessionThreadWantsExit = true;
    sessionThread->join();

    // destroy the window
    SDL_DestroyWindow(window);
}