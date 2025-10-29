
#include "FixedPoint/FixedMath.h"
#include <climits>  // For INT_MIN, INT_MAX

using namespace Phoenix;

static_assert(Deg2Rad(0.0f) == 0.0f);
static_assert(Deg2Rad(90.0f) == HALF_PI);
static_assert(Deg2Rad(180.0f) == PI);
static_assert(Deg2Rad(270.0f) == (PI + HALF_PI));
// Disabled due to floating point precision differences on Linux/Clang
// static_assert(Deg2Rad(360.0f).Value == TWO_PI.Value);

static_assert(Rad2Deg(0.0f) == 0.0f);
static_assert(Rad2Deg(HALF_PI) == 90.0f);
static_assert(Rad2Deg(PI) == 180.0f);
static_assert(Rad2Deg(PI + HALF_PI) == 270.0f);
// Disabled due to floating point precision differences on Linux/Clang
// static_assert(Rad2Deg(TWO_PI) == 360.0f);

static_assert(Abs_(-1) == 1);
static_assert(Abs_(-2) == 2);
static_assert(Abs_(1) == 1);
static_assert(Abs_(2) == 2);
static_assert(Abs_(INT_MIN + 1) == INT_MAX);

// static_assert(Atan2(0, 0) == 0);
// static_assert(Atan2(0, 1) == 0);
// static_assert(Atan2(1, 1).Value == 4685082);
// static_assert(Atan2(1, 0).Value == Angle(1.5715753).Value);
// static_assert(Atan2(1, -1).Value == Angle(2.3561921).Value);
// static_assert(Atan2(0, -1) == -PI);
// static_assert(Atan2(-1, -1) == Angle(-2.36049));
// static_assert(Atan2(-1, 0) == Angle(-1.57084));
// static_assert(Atan2(-1, 1) == Angle(-0.785398));
// static_assert(Atan2_<1024, int32>(1, 1) == TFixed<1024>::ToFixedValue(0.785398));
// static_assert(Atan2_<1024, int32>(1, 0) == TFixed<1024>::ToFixedValue(1.5708));
// static_assert(Atan2_<1024, int32>(1, -1) == TFixed<1024>::ToFixedValue(2.35619));
// static_assert(Atan2_<1024, int32>(0, -1) == TFixed<1024>::ToFixedValue(3.14159));
// static_assert(Atan2_<1024, int32>(-1, -1) == TFixed<1024>::ToFixedValue(-2.35619));
// static_assert(Atan2_<1024, int32>(-1, 0) == TFixed<1024>::ToFixedValue(-1.5708));
// static_assert(Atan2_<1024, int32>(-1, 1) == TFixed<1024>::ToFixedValue(-0.785398));

// static_assert(Sqrt(Distance(1.0)).Value == Distance(1.0).Value);
// static_assert(Sqrt(Distance(4.0)) == Distance(2.0));
// static_assert(Sqrt(Distance(16.0)) == Distance(4.0));
// static_assert(Sqrt(Distance(8.0*8.0)).Value == Distance(8.0).Value);
// static_assert(Sqrt(Distance(16.0*16.0)) == Distance(16.0));
// static_assert(Sqrt(Distance(32.0*32.0)) == Distance(32.0));
// static_assert(Sqrt(Distance(64.0*64.0)) == Distance(64.0));
// static_assert(Sqrt(Distance(128.0*128.0)) == Distance(128.0));