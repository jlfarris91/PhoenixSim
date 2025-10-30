
#pragma once

#include "FixedPoint.h"

namespace Phoenix
{
    using Value = TFixed<12>;
    using InvValue = TInvFixed2<Value>;
    using Distance = TFixed<12>;
    using Time = Fixed32_4;
    using DeltaTime = TInvFixed2<Time>;
    using Speed = Fixed32_16;
    using Angle = TFixed<20>;

    static constexpr Angle PI = static_cast<Angle::QT>(3294198);
    static constexpr Angle TWO_PI = static_cast<Angle::QT>(PI.Value * 2);
    static constexpr Angle FOUR_PI = static_cast<Angle::QT>(PI.Value * 4);
    static constexpr Angle HALF_PI = static_cast<Angle::QT>(PI.Value / 2);
    static constexpr TInvFixed2<Angle> INV_PI = static_cast<Angle::QT>(PI.Value);
    static constexpr TInvFixed2<Angle> INV_TWO_PI = static_cast<Angle::QT>(TWO_PI.Value);

    static constexpr Angle DEG_45 = static_cast<Angle::QT>(47185920);
    static constexpr Angle DEG_90 = static_cast<Angle::QT>(DEG_45.Value * 2);
    static constexpr Angle DEG_135 = static_cast<Angle::QT>(DEG_45.Value * 3);
    static constexpr Angle DEG_180 = static_cast<Angle::QT>(DEG_45.Value * 4);
    static constexpr Angle DEG_225 = static_cast<Angle::QT>(DEG_45.Value * 5);
    static constexpr Angle DEG_270 = static_cast<Angle::QT>(DEG_45.Value * 6);
    static constexpr Angle DEG_315 = static_cast<Angle::QT>(DEG_45.Value * 7);
    static constexpr Angle DEG_360 = static_cast<Angle::QT>(DEG_45.Value * 8);
    static constexpr TInvFixed2<Angle> INV_DEG_180 = static_cast<Angle::QT>(DEG_180.Value);

    constexpr Angle Deg2Rad(Angle d)
    {
        auto v = int64(d.Value) * PI.Value;
        auto v2 = v / DEG_180.Value;
        return Q64(v2);
    }

    constexpr Angle Rad2Deg(Angle r)
    {
        auto v = int64(r.Value) * DEG_180.Value;
        auto v2 = v / PI.Value;
        return Q64(v2);
    }
}
