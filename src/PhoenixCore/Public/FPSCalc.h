
#pragma once

#include <algorithm>
#include <limits>

#include "Platform.h"

namespace Phoenix
{
    struct FPSCalc
    {
        static constexpr uint32 NumFrames = 60;

        uint64 TotalTicks = 0;
        sys_clock_t Time;

        double SecPerFrameAccum = 0.0;
        double SecPerFrame[NumFrames] = { 0.0 };
        uint32 SecPerFrameIdx = 0;
        uint32 SecPerFrameCount = 0;

        double Framerate = 0.0;

        FPSCalc()
        {
            Reset();
        }

        double GetFramerate() const
        {
            return Framerate;
        }

        double GetFPS() const
        {
            return 1000.0f / Framerate;
        }

        void Reset()
        {
            TotalTicks = 0;
            Time = PHX_SYS_CLOCK_NOW();
            SecPerFrameAccum = 0.0;
            memset(&SecPerFrame[0], 0, NumFrames * sizeof(double));
            SecPerFrameIdx = 0;
            SecPerFrameCount = 0;
            Framerate = 0.0;
        }

        double Tick()
        {
            ++TotalTicks;

            sys_clock_t currTime = PHX_SYS_CLOCK_NOW();
            auto deltaTime = currTime - Time;
            Time = currTime;
            double dt = std::chrono::duration<double>(deltaTime).count();

            SecPerFrameAccum += dt - SecPerFrame[SecPerFrameIdx];
            SecPerFrame[SecPerFrameIdx] = dt;
            SecPerFrameIdx = (SecPerFrameIdx + 1) % NumFrames;
            SecPerFrameCount = std::min(SecPerFrameCount + 1, NumFrames);
            Framerate = (SecPerFrameAccum > 0.0) ? (1.0 / (SecPerFrameAccum / (double)SecPerFrameCount)) : std::numeric_limits<double>::max();

            return Framerate;
        }
    };
}
