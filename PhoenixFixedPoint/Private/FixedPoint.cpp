
#include "Vector.h"

using namespace Phoenix;

static_assert(bit_width(1) == 1);
static_assert(bit_width(3) == 2);
static_assert(bit_width(5) == 3);
static_assert(bit_width(64) == 7);

static_assert(Fixed16(0.5f).Value == Fixed16::D / 2);
static_assert(Fixed16(1.0f).Value == Fixed16::D);
static_assert(Fixed16(2.0f).Value == Fixed16::D * 2);

static_assert(Fixed16(1.0f).Value == Fixed16::ToFixedValue(1.0f));
static_assert(1.0f == Fixed16::FromFixedValue<float>(Fixed16::D));

static_assert((int32)Fixed16(123.123f) == 123);
static_assert((uint32)Fixed16(123.123f) == 123);
static_assert((int64)Fixed16(123.123f) == 123);
static_assert((uint64)Fixed16(123.123f) == 123);

//
// Addition
//

static_assert(Fixed16(1.0f) + Fixed16(1.0f) == 2);
static_assert(Fixed16(1.0f) + Fixed16(1.0f) == 2.0f);
static_assert(Fixed16(1.0f) + Fixed16(1.0f) == Fixed16(2.0f));

static_assert(Fixed16(1.0f) + 1.0f == 2);
static_assert(Fixed16(1.0f) + 1.0f == 2.0f);
static_assert(Fixed16(1.0f) + 1.0f == Fixed16(2.0f));

static_assert(1.0f + Fixed16(1.0f) == 2);
static_assert(1.0f + Fixed16(1.0f) == 2.0f);
static_assert(1.0f + Fixed16(1.0f) == Fixed16(2.0f));

//
// Subtraction
//

static_assert(Fixed16(10.0f) - Fixed16(5.0f) == 5);
static_assert(Fixed16(10.0f) - Fixed16(5.0f) == 5.0f);
static_assert(Fixed16(10.0f) - Fixed16(5.0f) == Fixed16(5.0f));

static_assert(Fixed16(5.0f) - Fixed16(10.0f) == -5);
static_assert(Fixed16(5.0f) - Fixed16(10.0f) == -5.0f);
static_assert(Fixed16(5.0f) - Fixed16(10.0f) == Fixed16(-5.0f));

static_assert(Fixed16(10.0f) - 5.0f == 5);
static_assert(Fixed16(10.0f) - 5.0f == 5.0f);
static_assert(Fixed16(10.0f) - 5.0f == Fixed16(5.0f));

static_assert(Fixed16(5.0f) - 10.0f == -5);
static_assert(Fixed16(5.0f) - 10.0f == -5.0f);
static_assert(Fixed16(5.0f) - 10.0f == Fixed16(-5.0f));

static_assert(10.0f - Fixed16(5.0f) == 5);
static_assert(10.0f - Fixed16(5.0f) == 5.0f);
static_assert(10.0f - Fixed16(5.0f) == Fixed16(5.0f));

static_assert(5.0f - Fixed16(10.0f) == -5);
static_assert(5.0f - Fixed16(10.0f) == -5.0f);
static_assert(5.0f - Fixed16(10.0f) == Fixed16(-5.0f));

//
// Multiplication
//

static_assert(Fixed16(5.0f) * Fixed16(2.0f) == 10);
static_assert(Fixed16(5.0f) * Fixed16(2.0f) == 10.0f);
static_assert(Fixed16(5.0f) * Fixed16(2.0f) == Fixed16(10.0f));

static_assert(Fixed16(1.0f) * 2 == 2);
static_assert(Fixed16(5.0f) * 2.0f == 10);
static_assert(Fixed16(5.0f) * 2.0f == 10.0f);
static_assert(Fixed16(5.0f) * 2.0f == Fixed16(10.0f));

static_assert(5.0f * Fixed16(2.0f) == 10);
static_assert(5.0f * Fixed16(2.0f) == 10.0f);
static_assert(5.0f * Fixed16(2.0f) == Fixed16(10.0f));

static_assert(TInvFixed2<Fixed16>(Fixed16(10.0f)).Value == Fixed16::D * 10);
static_assert((Fixed16(10) * TInvFixed2<Fixed16>(0.1f)).Value == Fixed16(1.0f).Value);

//
// Division
//

constexpr Fixed16 test10 = Fixed16(10.0f);
constexpr Fixed16 test1 = Fixed16(1.0f);
constexpr Fixed16 test2 = Fixed16(2.0f);
constexpr Fixed16 test5 = Fixed16(5.0f);
constexpr auto asdf = test10 / test2;

static_assert((test10 / test2).Value == test5.Value);

static_assert(Fixed16(10.0f) / Fixed16(2.0f) == 5.0f);
static_assert(Fixed16(10.0f) / Fixed16(2.0f) == 5.0f);
static_assert(Fixed16(10.0f) / Fixed16(2.0f) == Fixed16(5.0f));

constexpr Fixed20 test90 = Fixed20(90);
constexpr auto asdfsadf = test90 / test2;

static_assert((Fixed20(90.0f) / Fixed16(2.0f)).Value == Fixed20(45).Value);

constexpr Fixed16 testNeg1 = -1;
constexpr Fixed16 testNeg2 = -2;
constexpr auto asfdf = testNeg1 / test2;
constexpr auto asfd2f = test1 / testNeg2;

static_assert(Fixed16(1.0f) / 2.0f == 0.5f);
static_assert(Fixed16(1.5f) / 2.0f == 0.75f);
static_assert(Fixed16(-1.0f) / 2.0f == -0.5f);
static_assert(Fixed16(-1.5f) / 2.0f == -0.75f);
static_assert(Fixed16(1.0f) / -2.0f == -0.5f);
static_assert(Fixed16(1.5f) / -2.0f == -0.75f);

static_assert(OneDivBy<Fixed16>(10.0f).Value == Fixed16(10.0f).Value);
static_assert(Fixed16(10) / OneDivBy<Fixed16>(10.0f) == 100.0f);

constexpr auto a = OneDivBy<Fixed16>(2);
constexpr auto b = OneDivBy<Fixed16>(4);
constexpr auto c = int64(b.Value) - a.Value;
constexpr auto d = int64(a.Value) * b.Value;
constexpr auto e = d / c;
constexpr auto f = OneDivBy<Fixed16>(2) + OneDivBy<Fixed8>(2);
constexpr auto g = OneDivBy<Fixed16>(2) - OneDivBy<Fixed8>(2);
static_assert((OneDivBy<Fixed16>(2) + OneDivBy<Fixed16>(4)).Value == Fixed16(1.33333f).Value);
static_assert((OneDivBy<Fixed16>(2) - OneDivBy<Fixed16>(2)).Value == 0);
static_assert((OneDivBy<Fixed16>(2) - OneDivBy<Fixed16>(4)).Value == Fixed16(4.0f).Value);