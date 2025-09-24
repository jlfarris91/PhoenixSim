#pragma once

#include "PhoenixSim.h"

namespace Phoenix
{
    union PHOENIXSIM_API  Data
    {
        int32 Int32;
        uint32 UInt32;
        FName Name;
        Value Value;
        Distance Distance;
        Angle Degrees;
        Speed Speed;
    };
    
    struct PHOENIXSIM_API  Action
    {
        Action() = default;

        FName Verb = FName::None;
        FName Sender = FName::None;
        Data Data[6] = { 0 };
    };
}
