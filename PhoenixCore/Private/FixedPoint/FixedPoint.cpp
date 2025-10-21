
#include "FixedPoint/FixedVector.h"

using namespace Phoenix;

static_assert(BitWidth(1) == 1);
static_assert(BitWidth(3) == 2);
static_assert(BitWidth(5) == 3);
static_assert(BitWidth(64) == 7);

static_assert(Fixed32_16(0.5f).Value == Fixed32_16::D / 2);
static_assert(Fixed32_16(1.0f).Value == Fixed32_16::D);
static_assert(Fixed32_16(2.0f).Value == Fixed32_16::D * 2);

static_assert(Fixed32_16(1.0f).Value == Fixed32_16::ToFixedValue(1.0f));
static_assert(1.0f == Fixed32_16::FromFixedValue<float>(Fixed32_16::D));

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
// constexpr auto i = 32767;
// constexpr auto ii = i*i;
// constexpr auto a = int64(i) * (1 << 16);
// constexpr auto aa = a * a;
//
// constexpr auto x = Distance(i);
// constexpr auto xx = x*x;
// constexpr auto x2 = Distance::ConvertToRaw(xx);
// constexpr auto x3 = Distance(Q64(x2));
// constexpr auto x4 = FixedUtils::Mult128(xx.Value, (1 << 16));
// constexpr auto x5 = FixedUtils::Div128(x4, 100);
// constexpr auto x6 = Distance(Q64(x5));
//
// constexpr auto s3 = TFixed<16>::ConvertToRaw(xx);
// constexpr auto asdf = TFixed<32, int64>(Q64(aa));
// constexpr auto asdf2 = FixedUtils::Mult128(int64(x.Value), x.Value);
// constexpr auto asdf3 = asdf2.NarrowToI64<0>();
// constexpr auto asdf4 = TFixed<16, int32>(Q64(asdf3));
// constexpr auto asdfasdf = TFixed<16>(xx);
// constexpr auto z16 = TFixed<16>(512);
// constexpr auto z8 = TFixed<8>(512);
// constexpr auto z1 = TFixed<16>(Q64(TFixed<16>::ConvertToRaw(z16)));
// constexpr auto z2 = TFixed<16>(Q64(TFixed<16>::ConvertToRaw(z8)));

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