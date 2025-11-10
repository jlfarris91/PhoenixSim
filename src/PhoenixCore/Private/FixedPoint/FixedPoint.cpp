
#include "FixedPoint/FixedVector.h"
#include <climits>  // For INT_MIN, INT_MAX

using namespace Phoenix;

static_assert(BitWidth(1) == 1);
static_assert(BitWidth(3) == 2);
static_assert(BitWidth(5) == 3);
static_assert(BitWidth(64) == 7);

static_assert(Fixed32_16(0.5f).Value == Fixed32_16::D / 2);
static_assert(Fixed32_16(1.0f).Value == Fixed32_16::D);
static_assert(Fixed32_16(2.0f).Value == Fixed32_16::D * 2);

static_assert(Fixed32_16(1.0f).Value == Fixed32_16::ConvertToQ(1.0f));
static_assert(1.0f == Fixed32_16::ConvertFromQ<float>(Fixed32_16::D));

static_assert((int32)Fixed32_16(123.123f) == 123);
static_assert((uint32)Fixed32_16(123.123f) == 123);
static_assert((int64)Fixed32_16(123.123f) == 123);
static_assert((uint64)Fixed32_16(123.123f) == 123);

static_assert(ChkAdd_<int32>(INT_MAX, 0).Value == INT_MAX);
static_assert(ChkAdd_<int32>(0, INT_MAX).Value == INT_MAX);
static_assert(ChkAdd_<int32>(INT_MAX, 1).bOverflowed);
static_assert(ChkAdd_<int32>(INT_MIN, 0).Value == INT_MIN);
static_assert(ChkAdd_<int32>(0, INT_MIN).Value == INT_MIN);
static_assert(ChkAdd_<int32>(INT_MIN, -1).bOverflowed);

//
// Addition
//

static_assert(Fixed32_16(1.0f) + Fixed32_16(1.0f) == 2);
static_assert(Fixed32_16(1.0f) + Fixed32_16(1.0f) == 2.0f);
static_assert(Fixed32_16(1.0f) + Fixed32_16(1.0f) == Fixed32_16(2.0f));

static_assert(Fixed32_16(1.0f) + 1.0f == 2);
static_assert(Fixed32_16(1.0f) + 1.0f == 2.0f);
static_assert(Fixed32_16(1.0f) + 1.0f == Fixed32_16(2.0f));

static_assert(1.0f + Fixed32_16(1.0f) == 2);
static_assert(1.0f + Fixed32_16(1.0f) == 2.0f);
static_assert(1.0f + Fixed32_16(1.0f) == Fixed32_16(2.0f));

//
// Subtraction
//

static_assert(Fixed32_16(10.0f) - Fixed32_16(5.0f) == 5);
static_assert(Fixed32_16(10.0f) - Fixed32_16(5.0f) == 5.0f);
static_assert(Fixed32_16(10.0f) - Fixed32_16(5.0f) == Fixed32_16(5.0f));

static_assert(Fixed32_16(5.0f) - Fixed32_16(10.0f) == -5);
static_assert(Fixed32_16(5.0f) - Fixed32_16(10.0f) == -5.0f);
static_assert(Fixed32_16(5.0f) - Fixed32_16(10.0f) == Fixed32_16(-5.0f));

static_assert(Fixed32_16(10.0f) - 5.0f == 5);
static_assert(Fixed32_16(10.0f) - 5.0f == 5.0f);
static_assert(Fixed32_16(10.0f) - 5.0f == Fixed32_16(5.0f));

static_assert(Fixed32_16(5.0f) - 10.0f == -5);
static_assert(Fixed32_16(5.0f) - 10.0f == -5.0f);
static_assert(Fixed32_16(5.0f) - 10.0f == Fixed32_16(-5.0f));

static_assert(10.0f - Fixed32_16(5.0f) == 5);
static_assert(10.0f - Fixed32_16(5.0f) == 5.0f);
static_assert(10.0f - Fixed32_16(5.0f) == Fixed32_16(5.0f));

static_assert(5.0f - Fixed32_16(10.0f) == -5);
static_assert(5.0f - Fixed32_16(10.0f) == -5.0f);
static_assert(5.0f - Fixed32_16(10.0f) == Fixed32_16(-5.0f));

//
// Multiplication
//
//

static_assert(Fixed32_16(5.0f) * Fixed32_16(2.0f) == 10);
static_assert(Fixed32_16(5.0f) * Fixed32_16(2.0f) == 10.0f);
static_assert(Fixed32_16(5.0f) * Fixed32_16(2.0f) == Fixed32_16(10.0f));

static_assert(Fixed32_16(1.0f) * 2 == 2);
static_assert(Fixed32_16(5.0f) * 2.0f == 10);
static_assert(Fixed32_16(5.0f) * 2.0f == 10.0f);
static_assert(Fixed32_16(5.0f) * 2.0f == Fixed32_16(10.0f));

static_assert(5.0f * Fixed32_16(2.0f) == 10);
static_assert(5.0f * Fixed32_16(2.0f) == 10.0f);
static_assert(5.0f * Fixed32_16(2.0f) == Fixed32_16(10.0f));

//
// Division
//

static_assert(Fixed32_16(10.0f) / Fixed32_16(2.0f) == 5.0f);
static_assert(Fixed32_16(10.0f) / Fixed32_16(2.0f) == 5.0f);
static_assert(Fixed32_16(10.0f) / Fixed32_16(2.0f) == Fixed32_16(5.0f));

static_assert((Fixed32_20(90.0f) / Fixed32_16(2.0f)).Value == Fixed32_20(45).Value);

static_assert(Fixed32_16(1.0f) / 2.0f == 0.5f);
static_assert(Fixed32_16(1.5f) / 2.0f == 0.75f);
static_assert(Fixed32_16(-1.0f) / 2.0f == -0.5f);
static_assert(Fixed32_16(-1.5f) / 2.0f == -0.75f);
static_assert(Fixed32_16(1.0f) / -2.0f == -0.5f);
static_assert(Fixed32_16(1.5f) / -2.0f == -0.75f);

static_assert(OneDivBy<Fixed32_16>(10.0f).Value == Fixed32_16(10.0f).Value);
static_assert(Fixed32_16(10) / OneDivBy<Fixed32_16>(10.0f) == 100.0f);

static_assert((OneDivBy<Fixed32_16>(2) + OneDivBy<Fixed32_16>(4)).Value == Fixed32_16(1.33333f).Value);
static_assert((OneDivBy<Fixed32_16>(2) - OneDivBy<Fixed32_16>(2)).Value == 0);
static_assert((OneDivBy<Fixed32_16>(2) - OneDivBy<Fixed32_16>(4)).Value == Fixed32_16(4.0f).Value);