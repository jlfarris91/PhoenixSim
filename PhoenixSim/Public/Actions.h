#pragma once

#include "FixedPoint.h"
#include "PhoenixSim.h"

namespace Phoenix
{
    PHOENIXSIM_API union Data
    {
        int32 Int32;
        uint32 UInt32;
        FName Name;
        Value Value;
        Distance Distance;
        Degrees Degrees;
        Speed Speed;
        Mass Mass;
    };
    
    PHOENIXSIM_API struct Action
    {
        Action() = default;

        FName Verb = FName::None;
        FName Sender = FName::None;
        Data Data[6] = { 0 };
    };
}
