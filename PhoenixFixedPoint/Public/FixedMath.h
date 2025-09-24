
#pragma once

#include <climits>

#include "FixedPoint.h"
#include "CosTable.h"
#include "FixedMath.h"

namespace Phoenix
{
    namespace FixedMath
    {
        static constexpr Angle PI = 3.14159f;
        static constexpr auto TWO_PI = PI * 2;
        static constexpr auto PI_2 = PI / 2;
        static constexpr auto PI_4 = PI / 4;

        static constexpr auto DEG_45 = Angle(45.0f);
        static constexpr auto DEG_90 = Angle(90.0f);
        static constexpr auto DEG_135 = Angle(135.0f);
        static constexpr auto DEG_180 = Angle(180.0f);
        static constexpr auto DEG_225 = Angle(225.0f);
        static constexpr auto DEG_270 = Angle(270.0f);
        static constexpr auto DEG_315 = Angle(315.0f);
        static constexpr auto DEG_360 = Angle(360.0f);

        constexpr Angle Deg2Rad(Angle d)
        {
            auto v = int64(d.Value) * PI.Value;
            auto v2 = v / DEG_180.Value;
            return Q32(v2);
        }

        constexpr Angle Rad2Deg(Angle r)
        {
            auto v = int64(r.Value) * DEG_180.Value;
            auto v2 = v / PI.Value;
            return Q32(v2);
        }

        static_assert(PI.Value == 18740314);
        static_assert(PI_2.Value == 18740314 / 2);
        static_assert(PI_4.Value == 18740314 / 4);
        static_assert(TWO_PI.Value == 18740314 * 2);

        static_assert(Deg2Rad(0.0f) == 0.0f);
        static_assert(Deg2Rad(90.0f) == PI_2);
        static_assert(Deg2Rad(180.0f) == PI);
        static_assert(Deg2Rad(270.0f).Value == (PI + PI_2).Value - 1);
        static_assert(Deg2Rad(360.0f).Value == TWO_PI.Value);

        template <uint32 S = 24>
        struct CordicVector
        {
            struct Vec2
            {
                int32 x; int32 y;
            };
            
            // atan(1 / pow(2, i)) * Td, where Td == Angle::D (0x10000000 / 45)
            static constexpr int32 ARCTAN_TABLE[]
            {
                4685082, 2765765, 1461354, 741806, 372342, 186352, 93199, 46602, 
                23301, 11650, 5825, 2912, 1456, 728, 364, 182, 
                91, 45, 22, 11, 5, 2, 1, 0, 
            };

            static constexpr Vec2 Rotate(int32 r0, int32 t0)
            {
                
            }

            static constexpr Vec2 Vector(int32 x0, int32 y0)
            {
                static_assert(S <= 24);

                int64 x = x0;
                int64 y = y0;
                int64 z = 0;

                if (x < 0)
                {
                    x = -x;
                    y = -y;

                    if (y < 0)
                    {
                        z += PI.Value;
                    } 
                    else
                    {
                        z -= PI.Value;
                    }
                }

                for (uint32 i = 0; i < S && y != 0; ++i)
                {
                    int64 d = y > 0 ? 1 : -1;
                    int64 nx = x + d * (y >> i);
                    int64 ny = y - d * (x >> i);
                    int64 nz = z + d * ARCTAN_TABLE[i];
                    x = nx;
                    y = ny;
                    z = nz;
                }
        
                return { (int32)(x0 + x), (int32)z };
            }
        };

        template <class T>
        constexpr static T Abs_(T value)
        {
            constexpr T cBits = (sizeof(T) << 3) - 1;
            return (value ^ (value >> cBits)) - (value >> cBits);
        }

        static_assert(Abs_(-1) == 1);
        static_assert(Abs_(-2) == 2);
        static_assert(Abs_(1) == 1);
        static_assert(Abs_(2) == 2);
        static_assert(Abs_(INT_MIN + 1) == INT_MAX);

        template <uint32 Td, class T>
        constexpr static TFixed<Td, T> Abs(const TFixed<Td, T>& value)
        {
            return TFixedQ_T<T>(Abs_(value.Value));
        }

        constexpr Angle Atan2(const Distance& y, const Distance& x)
        {
            return TFixedQ_T<Angle::StorageT>(CordicVector<>::Vector(x.Value, y.Value).y);
        }

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

        template <int32 B>
        constexpr int32 isqrt(int32 x)
        {
            if (x <= 0) return 0;

            // Guess half of x + 1
            int32 r = (x + 1) >> 1;
            int32 iterations = bit_width(x) >> 1;

            for (int i = 0; i < iterations; i++)
            {
                int32 div = int32((int64(x) << B) / r);
                r = (r + div) >> 1;
            }

            return r;
        }

        static_assert(isqrt<Value::B>(Value(1.0).Value) == Value(1.0).Value);
        static_assert(isqrt<Value::B>(Value(4.0).Value) == Value(2.0).Value);
        static_assert(isqrt<Value::B>(Value(16.0).Value) == Value(4.0).Value);
        static_assert(isqrt<Value::B>(Value(8.0*8.0).Value) == Value(8.0).Value);
        static_assert(isqrt<Value::B>(Value(16.0*16.0).Value) == Value(16.0).Value);
        static_assert(isqrt<Value::B>(Value(32.0*32.0).Value) == Value(32.0).Value);
        static_assert(isqrt<Value::B>(Value(64.0*64.0).Value) == Value(64.0).Value);
        static_assert(isqrt<Value::B>(Value(128.0*128.0).Value) == Value(128.0).Value);
        static_assert(isqrt<Value::B>(Value(1024.0*1024.0).Value) == Value(1024.0).Value);

        constexpr Value Sqrt(Value value)
        {
            return Q32(isqrt<Value::B>(value.Value));
        }

        constexpr int32 modp(int32 a, int32 s)
        {
            while (a > s) a -= s;
            while (a < 0) a += s;
            return a;
        }

        constexpr Value Cos(Angle angle)
        {
            angle = Q32(modp(angle.Value, TWO_PI.Value));
            uint32 quadrant = (uint32)(angle * 4 / TWO_PI);
            angle = Q32(angle.Value % PI_2.Value);
            Value a = angle / PI_2;
            int64 idx = (int64)(a * COS_TABLE_LEN);
            Value val = Q32(COS_TABLE[idx]);
            switch (quadrant)
            {
                case 0:  return val;
                case 1:  return Q32(-COS_TABLE[COS_TABLE_LEN - idx]);
                case 2:  return -val;
                case 3:  return Q32(COS_TABLE[COS_TABLE_LEN - idx]);
                default: return 0;
            }
        }

        constexpr Value Sin(Angle angle)
        {
            angle = Q32(modp(angle.Value, TWO_PI.Value));
            uint32 quadrant = (uint32)(angle * 4 / TWO_PI);
            angle = Q32(angle.Value % PI_2.Value);
            Value a = angle / PI_2;
            int64 idx = (int64)(a * COS_TABLE_LEN);
            Value val = Q32(COS_TABLE[COS_TABLE_LEN - idx]);
            switch (quadrant)
            {
                case 0:  return val;
                case 1:  return Q32(COS_TABLE[idx]);
                case 2:  return -val;
                case 3:  return Q32(-COS_TABLE[idx]);
                default: return 0;
            }
        }

        constexpr Angle AngleBetween(Angle a, Angle b)
        {
            Angle d = Abs(a - b);
            if (d > PI)
            {
                d = TWO_PI - d;
            }
            return d;
        }

        template <uint32 Td, class T>
        constexpr TFixed<Td, T> Min(const TFixed<Td, T>& a, const TFixed<Td, T>& b)
        {
            return a < b ? a : b;
        }

        template <uint32 Td, class T>
        constexpr TFixed<Td, T> Max(const TFixed<Td, T>& a, const TFixed<Td, T>& b)
        {
            return a > b ? a : b;
        }
    }
}
