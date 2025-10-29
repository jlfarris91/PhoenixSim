
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
    using Angle = Fixed32_20;

    static constexpr double _pi_d = 3.1415926535897932384626433832795f;
    static constexpr Angle PI = _pi_d;
    static constexpr Angle TWO_PI = 6.283185307179586476925286766559;
    static constexpr Angle FOUR_PI = 12.566370614359172953850573533118;
    static constexpr Angle HALF_PI = 1.5707963267948966192313216916398;
    static constexpr Angle PI_4 = 0.78539816339744830961566084581988;
    static constexpr auto INV_PI = OneDivBy(PI);
    static constexpr auto INV_TWO_PI = OneDivBy(TWO_PI);

    static constexpr Angle DEG_45 = 45.0f;
    static constexpr Angle DEG_90 = 90.0f;
    static constexpr Angle DEG_135 = 135.0f;
    static constexpr Angle DEG_180 = 180.0f;
    static constexpr Angle DEG_225 = 225.0f;
    static constexpr Angle DEG_270 = 270.0f;
    static constexpr Angle DEG_315 = 315.0f;
    static constexpr Angle DEG_360 = 360.0f;
    static constexpr auto INV_DEG_180 = OneDivBy(DEG_180);

    static constexpr auto DEG_TO_RAD = PI * OneDivBy(DEG_180);
    static constexpr auto RAD_TO_DEG = DEG_180 * INV_PI;

    static constexpr Value SQRT2 = 1.4142135623730950488016887242097f;
    static constexpr Value SQRT3 = 1.7320508075688772935274463415059f;
    static constexpr auto INV_SQRT2 = OneDivBy(SQRT2);
    static constexpr auto INV_SQRT3 = OneDivBy(SQRT3);

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
