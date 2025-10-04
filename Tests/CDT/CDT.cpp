
#define NOMINMAX

#include <ctime>
#include <windows.h>
#include <fstream>

#include "Name.h"
#include "MortonCode.h"
#include "FixedPoint/FixedTransform.h"
#include "Mesh/Mesh2.h"

#define SDL_MAIN_USE_CALLBACKS
#include <queue>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include "MeshPath.h"
#include "Optional.h"

using namespace Phoenix;

SDL_Window* GWindow = nullptr;
SDL_Renderer *GRenderer = nullptr;
int32 GWindowWidth = 800;
int32 GWindowHeight = 600;

uint32 GRendererFPSCounter = 0;
uint64 GRendererFPSTimer = 0;
float GRendererFPS = 0.0f;

TMap<uint8, bool> GMouseButtonStates;
TMap<SDL_Keycode, bool> GKeyStates;

DefaultFixedCDTMesh2 GMesh;
TArray<Vec2> GPoints;

TArray<Line2> GLines;
TOptional<Vec2> GLineStart, GLineEnd;

TMeshPath<decltype(GMesh)> GMeshPath;
TOptional<Vec2> GPathStart, GPathGoal;
TArray<Vec2> Path;

#define SHOW_FACE_IDS 1
#define SHOW_FACE_CIRCUMCIRCLES 0
#define SHOW_VERT_IDS 1
#define SHOW_VERT_CIRCLES 1
#define PATH_STEPPING 0

void LoadPoints()
{
    std::ifstream f("foobar.bin", std::ios::binary);

    if (!f.is_open())
        return;

    size_t numPoints = 0;
    f.read(reinterpret_cast<char*>(&numPoints), sizeof(size_t));

    for (size_t i = 0; i < numPoints; ++i)
    {
        Vec2 pt;
        f.read(reinterpret_cast<char*>(&pt.X.Value), sizeof(Vec2::ComponentT));
        f.read(reinterpret_cast<char*>(&pt.Y.Value), sizeof(Vec2::ComponentT));
        GPoints.push_back(pt);
    }
    
    size_t numLines;
    f.read(reinterpret_cast<char*>(&numLines), sizeof(size_t));

    for (size_t i = 0; i < numLines; ++i)
    {
        Vec2 start, end;
        f.read(reinterpret_cast<char*>(&start.X.Value), sizeof(Vec2::ComponentT));
        f.read(reinterpret_cast<char*>(&start.Y.Value), sizeof(Vec2::ComponentT));
        f.read(reinterpret_cast<char*>(&end.X.Value), sizeof(Vec2::ComponentT));
        f.read(reinterpret_cast<char*>(&end.Y.Value), sizeof(Vec2::ComponentT));
        GLines.emplace_back(start, end);
    }
}

void SavePoints()
{
    std::ofstream f("foobar.bin", std::ios::binary);

    if (!f.is_open())
        return;

    size_t numPoints = GPoints.size();
    f.write(reinterpret_cast<const char*>(&numPoints), sizeof(size_t));
    for (auto pt : GPoints)
    {
        f.write(reinterpret_cast<const char*>(&pt.X.Value), sizeof(Vec2::ComponentT));
        f.write(reinterpret_cast<const char*>(&pt.Y.Value), sizeof(Vec2::ComponentT));
    }

    size_t numLines = GLines.size();
    f.write(reinterpret_cast<const char*>(&numLines), sizeof(size_t));
    for (auto line : GLines)
    {
        f.write(reinterpret_cast<const char*>(&line.Start.X.Value), sizeof(Vec2::ComponentT));
        f.write(reinterpret_cast<const char*>(&line.Start.Y.Value), sizeof(Vec2::ComponentT));
        f.write(reinterpret_cast<const char*>(&line.End.X.Value), sizeof(Vec2::ComponentT));
        f.write(reinterpret_cast<const char*>(&line.End.Y.Value), sizeof(Vec2::ComponentT));
    }

    f.close();
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    LoadPoints();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    GRendererFPSTimer = SDL_GetTicks();

    if (!SDL_CreateWindowAndRenderer("Phoenix - CDT", GWindowWidth, GWindowHeight, SDL_WINDOW_RESIZABLE, &GWindow, &GRenderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

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
    float mx, my;
    SDL_GetMouseState(&mx, &my);

    SDL_GetWindowSizeInPixels(GWindow, &GWindowWidth, &GWindowHeight);

    /* as you can see from this, rendering draws over whatever was drawn before it. */
    SDL_SetRenderDrawColor(GRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* black, full alpha */
    SDL_RenderClear(GRenderer);  /* start with a blank canvas. */

    /* Let's draw a single rectangle (square, really). */

    Vec2 mapSize(GWindowWidth, GWindowHeight);
    Vec2 windowCenter(GWindowWidth / 2.0f, GWindowHeight / 2.0f);

    SDL_SetRenderDrawColor(GRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

    static TArray<TTuple<uint32, uint32, uint32>> queryCodeColors;
    if (queryCodeColors.empty())
    {
        for (int32 i = 0; i < 1000; ++i)
        {
            queryCodeColors.emplace_back(rand() % 255, rand() % 255, rand() % 255);
        }
    }

    Distance cx = (float)windowCenter.X;
    Distance cy = (float)windowCenter.Y;
    Distance fmx = mx;
    Distance fmy = GWindowHeight - my;
    auto dx = fmx - cx;
    auto dy = fmy - cy;
    Vec2 m = { dx, dy };

    {
        Vec2 mapCenter(GWindowWidth >> 1, GWindowHeight >> 1);

        auto bl = Vec2(mapSize.X / -3, mapSize.Y / -3);
        auto br = Vec2(mapSize.X / +3, mapSize.Y / -3);
        auto tl = Vec2(mapSize.X / -3, mapSize.Y / +3);
        auto tr = Vec2(mapSize.X / +3, mapSize.Y / +3);

        //if (GMesh.Vertices.Num() - 4 != GEntityBodies.size())
        {
            GMesh.Reset();
            GMesh.InsertFace(bl, tr, tl, 1);
            GMesh.InsertFace(bl, br, tr, 2);

            for (const auto& point : GPoints)
            {
                CDT_InsertPoint(GMesh, point);
            }

            for (const auto& line : GLines)
            {
                CDT_InsertEdge(GMesh, line);
            }

            if (GLineStart.IsSet() && GLineEnd.IsSet() && !Vec2::Equals(*GLineStart, *GLineEnd))
            {
                CDT_InsertEdge(GMesh, Line2(*GLineStart, *GLineEnd));
            }
        }

        if (SHOW_VERT_CIRCLES)
        {
            for (auto vert : GMesh.Vertices)
            {
                auto x = mapCenter.X + vert.X;
                auto y = mapCenter.Y - vert.Y;
                SDL_RenderCircle(GRenderer, (float)x, (float)y, 10.0f, 10);
            }
        }

        for (auto edge : GMesh.HalfEdges)
        {
            if (!GMesh.Faces.IsValidIndex(edge.Face))
                continue;

            if (edge.bLocked)
                continue;

            SDL_SetRenderDrawColor(GRenderer, 50, 50, 50, SDL_ALPHA_OPAQUE);

            auto x0 = mapCenter.X + GMesh.Vertices[edge.VertA].X;
            auto y0 = mapCenter.Y - GMesh.Vertices[edge.VertA].Y;
            auto x1 = mapCenter.X + GMesh.Vertices[edge.VertB].X;
            auto y1 = mapCenter.Y - GMesh.Vertices[edge.VertB].Y;
            SDL_RenderLine(GRenderer, (float)x0, (float)y0, (float)x1, (float)y1);
        }

        for (auto edge : GMesh.HalfEdges)
        {
            if (!GMesh.Faces.IsValidIndex(edge.Face))
                continue;

            if (!edge.bLocked)
                continue;

            SDL_SetRenderDrawColor(GRenderer, 255, 0, 0, SDL_ALPHA_OPAQUE);

            auto x0 = mapCenter.X + GMesh.Vertices[edge.VertA].X;
            auto y0 = mapCenter.Y - GMesh.Vertices[edge.VertA].Y;
            auto x1 = mapCenter.X + GMesh.Vertices[edge.VertB].X;
            auto y1 = mapCenter.Y - GMesh.Vertices[edge.VertB].Y;
            SDL_RenderLine(GRenderer, (float)x0, (float)y0, (float)x1, (float)y1);
        }

        // Redraw the edges of the face the mouse is within so that they draw on top
        for (size_t i = 0; i < GMesh.Faces.Num(); ++i)
        {
            auto result = GMesh.PointInFace(int16(i), m);
            if (result.Result == EPointInFaceResult::Inside)
            {
                auto& color = queryCodeColors[i];
                auto colorR = uint8(std::get<0>(color) / 2);
                auto colorG = uint8(std::get<1>(color) / 2);
                auto colorB = uint8(std::get<2>(color) / 2);
                SDL_SetRenderDrawColor(GRenderer, colorR, colorG, colorB, SDL_ALPHA_OPAQUE);

                GMesh.ForEachHalfEdgeInFace(i, [&](const auto& halfEdge)
                {
                    auto x0 = mapCenter.X + GMesh.Vertices[halfEdge.VertA].X;
                    auto y0 = mapCenter.Y - GMesh.Vertices[halfEdge.VertA].Y;
                    auto x1 = mapCenter.X + GMesh.Vertices[halfEdge.VertB].X;
                    auto y1 = mapCenter.Y - GMesh.Vertices[halfEdge.VertB].Y;
                    SDL_RenderLine(GRenderer, (float)x0, (float)y0, (float)x1, (float)y1);
                });
            }
        }

        if (GLineStart.IsSet() && GLineEnd.IsSet() && !Vec2::Equals(*GLineStart,*GLineEnd))
        {
            SDL_SetRenderDrawColor(GRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            auto x0 = mapCenter.X + GLineStart->X;
            auto y0 = mapCenter.Y - GLineStart->Y;
            auto x1 = mapCenter.X + GLineEnd->X;
            auto y1 = mapCenter.Y - GLineEnd->Y;
            SDL_RenderLine(GRenderer, (float)x0, (float)y0, (float)x1, (float)y1);

            for (auto edge : GMesh.HalfEdges)
            {
                if (edge.Face == Index<uint16>::None)
                {
                    continue;
                }

                const Vec2& a = GMesh.Vertices[edge.VertA];
                const Vec2& b = GMesh.Vertices[edge.VertB];

                Vec2 pt;
                if (Vec2::Intersects(a, b, *GLineStart, *GLineEnd, pt))
                {
                    auto x0 = mapCenter.X + GMesh.Vertices[edge.VertA].X;
                    auto y0 = mapCenter.Y - GMesh.Vertices[edge.VertA].Y;
                    auto x1 = mapCenter.X + GMesh.Vertices[edge.VertB].X;
                    auto y1 = mapCenter.Y - GMesh.Vertices[edge.VertB].Y;
                    SDL_RenderLine(GRenderer, (float)x0, (float)y0, (float)x1, (float)y1);

                    auto ptx = mapCenter.X + pt.X;
                    auto pty = mapCenter.Y - pt.Y;
                    SDL_RenderCircle(GRenderer, (float)ptx, (float)pty, 10, 32);
                }
            }
        }

        if (GPathStart.IsSet())
        {
            SDL_SetRenderDrawColor(GRenderer, 0, 0, 100, SDL_ALPHA_OPAQUE);

            auto ptx = mapCenter.X + GPathStart->X;
            auto pty = mapCenter.Y - GPathStart->Y;
            SDL_RenderCircle(GRenderer, (float)ptx, (float)pty, 10, 32);
        }

        if (GPathGoal.IsSet())
        {
            SDL_SetRenderDrawColor(GRenderer, 0, 0, 100, SDL_ALPHA_OPAQUE);

            auto ptx = mapCenter.X + GPathGoal->X;
            auto pty = mapCenter.Y - GPathGoal->Y;
            SDL_RenderCircle(GRenderer, (float)ptx, (float)pty, 10, 32);
        }

        {
            for (uint16 halfEdgeIndex : GMeshPath.OpenSet)
            {
                SDL_SetRenderDrawColor(GRenderer, 0, 255, 0, SDL_ALPHA_OPAQUE);

                const auto& halfEdge = GMesh.HalfEdges[halfEdgeIndex];

                auto vertA = GMesh.Vertices[halfEdge.VertA];
                auto vertB = GMesh.Vertices[halfEdge.VertB];

                auto x0 = mapCenter.X + vertA.X;
                auto y0 = mapCenter.Y - vertA.Y;
                auto x1 = mapCenter.X + vertB.X;
                auto y1 = mapCenter.Y - vertB.Y;
                SDL_RenderLine(GRenderer, (float)x0, (float)y0, (float)x1, (float)y1);

                Vec2 center = Vec2::Midpoint(vertA, vertB);
                Vec2 d = (vertB - vertA).Normalized();
                Vec2 cross = Vec2(-d.Y, d.X);
                Vec2 f = center + cross * 10.0f;

                x0 = mapCenter.X + center.X;
                y0 = mapCenter.Y - center.Y;
                x1 = mapCenter.X + f.X;
                y1 = mapCenter.Y - f.Y;
                SDL_RenderLine(GRenderer, (float)x0, (float)y0, (float)x1, (float)y1);
            }

            SDL_SetRenderDrawColor(GRenderer, 0, 0, 255, SDL_ALPHA_OPAQUE);
            for (int32 i = 0; i < (int32)Path.size() - 1; ++i)
            {
                auto x0 = mapCenter.X + Path[i + 0].X;
                auto y0 = mapCenter.Y - Path[i + 0].Y;
                auto x1 = mapCenter.X + Path[i + 1].X;
                auto y1 = mapCenter.Y - Path[i + 1].Y;
                SDL_RenderLine(GRenderer, (float)x0, (float)y0, (float)x1, (float)y1);
            }
        }

        SDL_SetRenderDrawColor(GRenderer, 0, 100, 0, SDL_ALPHA_OPAQUE);

        if (SHOW_VERT_IDS)
        {
            constexpr float scale = 2.0f;
            SDL_SetRenderDrawColor(GRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_SetRenderScale(GRenderer, scale, scale);

            for (size_t i = 0; i < GMesh.Vertices.Num(); ++i)
            {
                const Vec2& pt = GMesh.Vertices[i];

                auto ptx = mapCenter.X + pt.X;
                auto pty = mapCenter.Y - pt.Y;

                char str[256] = { '\0' };
                sprintf_s(str, _countof(str), "%llu", i);
                SDL_RenderDebugText(GRenderer, (float)ptx / scale, (float)pty / scale, str);
            }

            SDL_SetRenderScale(GRenderer, 1.0f, 1.0f);
        }

        if (SHOW_FACE_IDS)
        {
            constexpr float scale = 2.0f;
            SDL_SetRenderDrawColor(GRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_SetRenderScale(GRenderer, scale, scale);

            for (int32 i = 0; i < GMesh.Faces.Num(); ++i)
            {
                if (!GMesh.Faces.IsValidIndex(i))
                    continue;

                const auto& face = GMesh.Faces[i];
                if (!GMesh.HalfEdges.IsValidIndex(face.HalfEdge))
                    continue;

                auto& color = queryCodeColors[i];
                SDL_SetRenderDrawColor(GRenderer, std::get<0>(color), std::get<1>(color), std::get<2>(color), SDL_ALPHA_OPAQUE);

                const auto& edge0 = GMesh.HalfEdges[face.HalfEdge];
                const auto& edge1 = GMesh.HalfEdges[edge0.Next];
                const auto& edge2 = GMesh.HalfEdges[edge1.Next];
            
                const Vec2& a = GMesh.Vertices[edge0.VertA];
                const Vec2& b = GMesh.Vertices[edge1.VertA];
                const Vec2& c = GMesh.Vertices[edge2.VertA];

                auto center = (a + b + c) / 3.0;
                center.X += mapCenter.X;
                center.Y = mapCenter.Y - center.Y;
        
                char str[256] = { '\0' };
                sprintf_s(str, _countof(str), "%llu", i);
                SDL_RenderDebugText(GRenderer, (float)center.X / scale, (float)center.Y / scale, str);
            }

            SDL_SetRenderScale(GRenderer, 1.0f, 1.0f);
        }

        if (SHOW_FACE_CIRCUMCIRCLES)
        {
            for (int32 i = 0; i < GMesh.Faces.Num(); ++i)
            {
                if (!GMesh.Faces.IsValidIndex(i))
                    continue;

                const auto& face = GMesh.Faces[i];
                if (!GMesh.HalfEdges.IsValidIndex(face.HalfEdge))
                    continue;

                auto& color = queryCodeColors[i];
                auto colorR = uint8(std::get<0>(color) / 2);
                auto colorG = uint8(std::get<1>(color) / 2);
                auto colorB = uint8(std::get<2>(color) / 2);
                SDL_SetRenderDrawColor(GRenderer, colorR, colorG, colorB, SDL_ALPHA_OPAQUE);

                const auto& e0 = GMesh.HalfEdges[face.HalfEdge];
                const auto& e1 = GMesh.HalfEdges[e0.Next];
                const auto& e2 = GMesh.HalfEdges[e1.Next];
        
                const Vec2& a = GMesh.Vertices[e0.VertA];
                const Vec2& b = GMesh.Vertices[e1.VertA];
                const Vec2& c = GMesh.Vertices[e2.VertA];
        
                auto aax = a.X * a.X;
                auto aay = a.Y * a.Y;
                auto a2 = a.X*a.X + a.Y*a.Y;
                auto b2 = b.X*b.X + b.Y*b.Y;
                auto c2 = c.X*c.X + c.Y*c.Y;
        
                auto d = 2 * (a.X * (b.Y - c.Y) + b.X * (c.Y - a.Y) + c.X * (a.Y - b.Y));
                if (d == 0)
                    continue;
        
                auto ux = (a2 * (b.Y - c.Y) + b2 * (c.Y - a.Y) + c2 * (a.Y - b.Y)) / d;
                auto uy = (a2 * (c.X - b.X) + b2 * (a.X - c.X) + c2 * (b.X - a.X)) / d;
                auto r1 = (ux - a.X)*(ux - a.X) + (uy - a.Y)*(uy - a.Y);

                if (r1 < 0)
                    continue;

                auto r = Sqrt(r1);
        
                ux = mapCenter.X + ux;
                uy = mapCenter.Y - uy;
        
                SDL_RenderCircle(GRenderer, (float)ux, (float)uy, (float)r, 32);
            }
        }
    }

    SDL_SetRenderDrawColor(GRenderer, 0, 255, 0, SDL_ALPHA_OPAQUE);

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

    RenderDebugText("%.0f, %.0f", mx, my)

    {
        RenderDebugText("F: %llu, E: %llu, V: %llu", GMesh.Faces.Num(), GMesh.HalfEdges.Num(), GMesh.Vertices.Num());
        
        for (int32 i = 0; i < GMesh.Faces.Num(); ++i)
        {
            if (!GMesh.Faces.IsValidIndex(i))
                continue;

            const auto& face = GMesh.Faces[i];
            if (!GMesh.HalfEdges.IsValidIndex(face.HalfEdge))
                continue;

            const auto& edge0 = GMesh.HalfEdges[face.HalfEdge];
            const auto& edge1 = GMesh.HalfEdges[edge0.Next];
            const auto& edge2 = GMesh.HalfEdges[edge1.Next];

            auto e0 = face.HalfEdge;
            auto e1 = edge0.Next;
            auto e2 = edge1.Next;

            auto v0 = edge0.VertA;
            auto v1 = edge1.VertA;
            auto v2 = edge2.VertA;

            auto et0 = edge0.Twin;
            auto et1 = edge1.Twin;
            auto et2 = edge2.Twin;

            auto result = GMesh.PointInFace(int16(i), m);
            switch (result.Result)
            {
                case EPointInFaceResult::Inside:
                    RenderDebugText("Inside %d - %u (%u/%u) -> %u (%u/%u) -> %u (%u/%u)", i, v0, e0, et0, v1, e1, et1, v2, e2, et2)
                    break;
                case EPointInFaceResult::OnEdge:
                    RenderDebugText("On Edge %d", result.OnEdgeIndex)
                    break;
                default: break;
            }
        }
    }

    {
        RenderDebugText("Start: %u", GMeshPath.StartFaceIndex)
        RenderDebugText("Goal: %u", GMeshPath.GoalFaceIndex)
        RenderDebugText("Steps: %u", GMeshPath.Steps)

        switch (GMeshPath.LastStepResult)
        {
            case TMeshPath<>::EStepResult::Continue:
                RenderDebugText("State: Continue")
                break;
            case TMeshPath<>::EStepResult::FoundPath:
                RenderDebugText("State: Found path!")
                break;
            case TMeshPath<>::EStepResult::Failed:
                RenderDebugText("State: Failed!")
                break;
        }

        RenderDebugText("Open Set: %llu", GMeshPath.OpenSet.Num())
        for (size_t i = 0; i < GMeshPath.OpenSet.Num(); ++i)
        {
            const decltype(GMeshPath)::Node& node = GMeshPath.FindOrAddNode(GMesh, GMeshPath.OpenSet[i]);
            RenderDebugText("  %llu: %u G:%f F:%f %u", i, GMeshPath.OpenSet[i], (float)node.G, (float)node.GetF(), node.FromEdge)
        }
    }

    SDL_SetRenderScale(GRenderer, 1.0f, 1.0f);

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

    auto mapCenterX = GWindowWidth >> 1;
    auto mapCenterY = GWindowHeight >> 1;

    float mx, my;
    SDL_GetMouseState(&mx, &my);

    SDL_FPoint mousePos = { mx, my };
    Vec2 mousePosSim = Vec2(mousePos.x - mapCenterX, -(mousePos.y - mapCenterY));

    auto onMouseDownOrMoved = [&]()
    {
        if (GMouseButtonStates.contains(SDL_BUTTON_LEFT) && GMouseButtonStates[SDL_BUTTON_LEFT])
        {
            GLineEnd = mousePosSim;
        }

        // Remove points
        if (GKeyStates.contains(SDLK_X) && GKeyStates[SDLK_X])
        {
            auto pt = GPoints.begin();
            for (; pt != GPoints.end();)
            {
                if ((*pt - mousePosSim).Length() < 64.0)
                {
                    pt = GPoints.erase(pt);
                }
                else
                    ++pt;
            }

            auto line = GLines.begin();
            for (; line != GLines.end();)
            {
                if (Line2::DistanceToLine(*line, mousePosSim) < 64.0)
                {
                    line = GLines.erase(line);
                }
                else
                    ++line;
            }
        }
    };

    if (event->type == SDL_EVENT_KEY_DOWN)
    {
        GKeyStates[event->key.key] = true;

        onMouseDownOrMoved();
    }

    if (event->type == SDL_EVENT_KEY_UP)
    {
        GKeyStates[event->key.key] = false;

        if (event->key.key == SDLK_SPACE && GMeshPath.LastStepResult == TMeshPath<>::EStepResult::Continue)
        {
            auto result = GMeshPath.Step(GMesh);
            if (result == TMeshPath<>::EStepResult::FoundPath)
            {
                Path.clear();
                GMeshPath.ResolvePath(GMesh, Path);
            }
        }
    }
    
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        GMouseButtonStates[event->button.button] = true;

        if (event->button.button == SDL_BUTTON_LEFT)
        {
            GLineStart = mousePosSim;
            GLineEnd.Reset();
        }

        onMouseDownOrMoved();
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        GMouseButtonStates[event->button.button] = false;

        if (event->button.button == SDL_BUTTON_LEFT)
        {
            if (GLineStart.IsSet() && GLineEnd.IsSet() &&
                (*GLineStart - *GLineEnd).Length() > 10.0f)
            {
                GLines.push_back(Line2(*GLineStart, *GLineEnd));
            }
            else if (GLineEnd.IsSet())
            {
                GPoints.push_back(*GLineEnd);
            }

            GLineStart.Reset();
            GLineEnd.Reset();
        }

        if (event->button.button == SDL_BUTTON_RIGHT)
        {
            if (GPathStart.IsSet() && GPathGoal.IsSet())
            {
                GPathStart.Reset();
                GPathGoal.Reset();
            }

            if (!GPathStart.IsSet())
                GPathStart = mousePosSim;
            else if (!GPathGoal.IsSet())
            {
                GPathGoal = mousePosSim;
                GMeshPath.FindPath(GMesh, *GPathStart, *GPathGoal, PATH_STEPPING);

                if (!PATH_STEPPING)
                {
                    if (GMeshPath.LastStepResult == TMeshPath<>::EStepResult::FoundPath)
                    {
                        Path.clear();
                        GMeshPath.ResolvePath(GMesh, Path);
                    }
                }
            }
        }
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        onMouseDownOrMoved();
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SavePoints();
   
    // destroy the window
    SDL_DestroyWindow(GWindow);
}