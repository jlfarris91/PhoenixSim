
#pragma once

#include <climits>

#include "FixedPoint.h"
#include "CosTable.h"
#include "FixedMath.h"

namespace Phoenix
{
    namespace FixedMath
    {
        static constexpr Angle PI = 3.1415926535897932384626433832795f;
        static constexpr Angle TWO_PI = 2 * PI;
        static constexpr Angle FOUR_PI = 4 * PI;
        static constexpr Angle HALF_PI = PI / 2;
        static constexpr Angle PI_4 = PI / 4;
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

        template <uint32 S = 24>
        struct CordicVector
        {
            struct Vec2
            {
                int32 x; int32 y;
            };
            
            // atan(1 / pow(2, i)) * Tb, where Tb == Angle::D (0x10000000 / 45)
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

        template <int32 Tb, class T>
        constexpr static TFixed<Tb, T> Abs(const TFixed<Tb, T>& value)
        {
            return TFixedQ_T<T>(Abs_(value.Value));
        }

        template <int32 Tb, class T>
        constexpr static TFixedSq_<Tb, int64> Square(const TFixed<Tb, T>& value)
        {
            return Q64(int64(value.Value) * value.Value);
        }

        template <int32 Tb, class T>
        constexpr static TFixed<Tb, T> Square2(const TFixed<Tb, T>& value)
        {
            return TFixed<Tb, T>(Square(value));
        }

        constexpr Angle Atan2(const Distance& y, const Distance& x)
        {
            return TFixedQ_T<Angle::ValueT>(CordicVector<>::Vector(x.Value, y.Value).y);
        }

        template <int32 Tb>
        constexpr int64 isqrt(int64 x)
        {
            if (x <= 0) return 0;

            // Guess half of x + 1
            int64 r = (x + 1) >> 1;
            int64 iterations = bit_width(x) >> 1;

            for (int i = 0; i < iterations; i++)
            {
                int64 div = int64((int64(x) << Tb) / r);
                r = (r + div) >> 1;
            }

            return r;
        }

        template <uint64 Tb, class T>
        constexpr TFixed<Tb, T> Sqrt(const TFixed<Tb, T>& value)
        {
            return TFixedQ_T<T>(isqrt<Tb>(value.Value));
        }

        constexpr int32 modp(int32 a, int32 s)
        {
            while (a > s) a -= s;
            while (a < 0) a += s;
            return a;
        }

        constexpr Value Cos(Angle angle)
        {
            angle = Q64(modp(angle.Value, TWO_PI.Value));
            uint32 quadrant = (uint32)(angle * 4 / TWO_PI);
            angle = Q64(angle.Value % HALF_PI.Value);
            Value a = angle / HALF_PI;
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
            angle = Q32(angle.Value % HALF_PI.Value);
            Value a = angle / HALF_PI;
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

        template <int32 Tb, class T, class U>
        constexpr TFixed<Tb, T> Min(const TFixed<Tb, T>& a, const U& b)
        {
            return a < b ? a : b;
        }

        template <int32 Tb, class T, class U>
        constexpr TFixed<Tb, T> Max(const TFixed<Tb, T>& a, const U& b)
        {
            return a > b ? a : b;
        }
    }
}
