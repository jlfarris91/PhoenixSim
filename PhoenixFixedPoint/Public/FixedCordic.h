
#pragma once

#include <complex>

#include "FixedTypes.h"

namespace Phoenix
{
    // Translated from: https://people.sc.fsu.edu/%7Ejburkardt/cpp_src/cordic/cordic.cpp
    namespace Cordic
    {
        constexpr int32 AnglesLen = 60;
        constexpr Angle Angles[AnglesLen] =
        {
            7.8539816339744830962E-01,
            4.6364760900080611621E-01,
            2.4497866312686415417E-01,
            1.2435499454676143503E-01,
            6.2418809995957348474E-02,
            3.1239833430268276254E-02,
            1.5623728620476830803E-02,
            7.8123410601011112965E-03,
            3.9062301319669718276E-03,
            1.9531225164788186851E-03,
            9.7656218955931943040E-04,
            4.8828121119489827547E-04,
            2.4414062014936176402E-04,
            1.2207031189367020424E-04,
            6.1035156174208775022E-05,
            3.0517578115526096862E-05,
            1.5258789061315762107E-05,
            7.6293945311019702634E-06,
            3.8146972656064962829E-06,
            1.9073486328101870354E-06,
            9.5367431640596087942E-07,
            4.7683715820308885993E-07,
            2.3841857910155798249E-07,
            1.1920928955078068531E-07,
            5.9604644775390554414E-08,
            2.9802322387695303677E-08,
            1.4901161193847655147E-08,
            7.4505805969238279871E-09,
            3.7252902984619140453E-09,
            1.8626451492309570291E-09,
            9.3132257461547851536E-10,
            4.6566128730773925778E-10,
            2.3283064365386962890E-10,
            1.1641532182693481445E-10,
            5.8207660913467407226E-11,
            2.9103830456733703613E-11,
            1.4551915228366851807E-11,
            7.2759576141834259033E-12,
            3.6379788070917129517E-12,
            1.8189894035458564758E-12,
            9.0949470177292823792E-13,
            4.5474735088646411896E-13,
            2.2737367544323205948E-13,
            1.1368683772161602974E-13,
            5.6843418860808014870E-14,
            2.8421709430404007435E-14,
            1.4210854715202003717E-14,
            7.1054273576010018587E-15,
            3.5527136788005009294E-15,
            1.7763568394002504647E-15,
            8.8817841970012523234E-16,
            4.4408920985006261617E-16,
            2.2204460492503130808E-16,
            1.1102230246251565404E-16,
            5.5511151231257827021E-17,
            2.7755575615628913511E-17,
            1.3877787807814456755E-17,
            6.9388939039072283776E-18,
            3.4694469519536141888E-18,
            1.7347234759768070944E-18
        };

        constexpr int32 KProdLen = 33;
        constexpr Fixed32_16 KProd[KProdLen]
        {
            0.70710678118654752440,
            0.63245553203367586640,
            0.61357199107789634961,
            0.60883391251775242102,
            0.60764825625616820093,
            0.60735177014129595905,
            0.60727764409352599905,
            0.60725911229889273006,
            0.60725447933256232972,
            0.60725332108987516334,
            0.60725303152913433540,
            0.60725295913894481363,
            0.60725294104139716351,
            0.60725293651701023413,
            0.60725293538591350073,
            0.60725293510313931731,
            0.60725293503244577146,
            0.60725293501477238499,
            0.60725293501035403837,
            0.60725293500924945172,
            0.60725293500897330506,
            0.60725293500890426839,
            0.60725293500888700922,
            0.60725293500888269443,
            0.60725293500888161574,
            0.60725293500888134606,
            0.60725293500888127864,
            0.60725293500888126179,
            0.60725293500888125757,
            0.60725293500888125652,
            0.60725293500888125626,
            0.60725293500888125619,
            0.60725293500888125617
        };

        // Shifts a fixed-point radian angle to lie between radian angle b and b+2PI
        constexpr Angle AngleShift(Angle a, Angle b = 0)
        {
            if (a.Value < b.Value)
            {
                return Q64(int64(b.Value) - (int64(b.Value) - a.Value) % TWO_PI.Value + TWO_PI.Value);
            }
            return Q64(int64(b.Value) + (int64(a.Value) - b.Value) % TWO_PI.Value);
        }

        static_assert(AngleShift(Deg2Rad(-45)) == Deg2Rad(315));
        static_assert(AngleShift(Deg2Rad(45)) == Deg2Rad(45));
        static_assert(AngleShift(Deg2Rad(405)) == Deg2Rad(45));

        // -1 <= t <= 1, n is the number of iterations
        template <class T>
        constexpr Angle ArcTan2(T y, T x, int32 n = 32)
        {
            using ValueT = int64;
            using AngleT = int64;

            ValueT xn = x.Value;
            ValueT yn = y.Value;
            AngleT theta = 0;
            AngleT angle = 0;

            int32 sign = 1;
            AngleT offset = 0;
            if (xn < 0 && yn < 0)
            {
                xn = -xn;
                yn = -yn;
                offset = -PI.Value;
            }
            else if (xn < 0)
            {
                xn = -xn;
                sign = -1;
                offset = PI.Value;
            }
            else if (yn < 0)
            {
                yn = -yn;
                sign = -1;
            }

            for (int32 i = 0; i < n; ++i)
            {
                int32 sigma = yn <= 0 ? 1 : -1;

                if (i < AnglesLen)
                {
                    angle = Angles[i].Value;
                }
                else
                {
                    angle >>= 1;
                }

                int64 x2 = xn - sigma * (yn >> i);
                int64 y2 = yn + sigma * (xn >> i);
                theta -= sigma * angle;

                xn = x2;
                yn = y2;
            }

            return Q64(theta * sign + offset);
        }

        template <class T>
        constexpr void CosSin(Angle a, T& c, T& s, int32 n = 32)
        {
            using ValueT = typename T::ValueT;
            using AngleT = Angle::ValueT;

            int32 sign;
            ValueT powerOfTwo = T::D;

            AngleT theta = AngleShift(a, -PI).Value;

            if (theta < -HALF_PI)
            {
                theta += PI.Value;
                sign = -1;
            }
            else if (HALF_PI < theta)
            {
                theta -= PI.Value;
                sign = -1;
            }
            else
            {
                sign = 1;
            }

            c = T::D;
            s = 0;

            AngleT angle = Angles[0].Value;

            for (int32 i = 0; i < n; ++i)
            {
                int32 sigma = theta < 0 ? -1 : 1;

                auto factor = sigma * powerOfTwo;

                auto c2 = c - factor * s;
                auto s2 = factor * c + s;

                c = c2;
                s = s2;

                theta -= sigma * angle;
                powerOfTwo >>= 1;

                if (i < AnglesLen)
                {
                    angle = Angles[i].Value;
                }
                else
                {
                    angle >>= 1;
                }
            }

            c = T(Q64(c * sign)) * KProd[(n < KProdLen ? n : KProdLen) - 1];
            s = T(Q64(c * sign)) * KProd[(n < KProdLen ? n : KProdLen) - 1];
        }

        template <class T>
        constexpr T Sqrt(T x, int32 n = 32)
        {
            if (x < 0.0)
            {
                return x.Value / 0;
            }

            if (x.Value == 0.0)
            {
                return T::QZero;
            }

            if (x.Value == Value::D)
            {
                return T::QOne;
            }

            T y = 0.0;
            T powerOfTwo = 1.0;

            if (x.Value < T::D)
            {
                while (x <= powerOfTwo * powerOfTwo)
                {
                    powerOfTwo = Q64(powerOfTwo.Value >> 1);
                }
                y = powerOfTwo;
            }
            else if (x.Value > T::D)
            {
                while (powerOfTwo * powerOfTwo <= x)
                {
                    powerOfTwo = Q64(powerOfTwo.Value << 1);
                }
                y = decltype(powerOfTwo)(Q64(powerOfTwo.Value >> 1));
            }

            for (int32 i = 0; i <= n; ++i)
            {
                powerOfTwo = Q64(powerOfTwo.Value >> 1);
                if ((y + powerOfTwo) * (y + powerOfTwo) <= x)
                {
                    y += powerOfTwo;
                }
            }

            return y;
        }

        template <class TX, class TY>
        struct TCordicVec
        {
            TX X;
            TY Y;
        };

        constexpr TCordicVec<double, double> RotateD(double x, double y, double a, int32 n = 32)
        {
            double xn = x;
            double yn = y;
            double z = a;
            double powerOfTwo = 1.0;

            for (int32 i = 0; i < n; ++i)
            {
                int32 sign = z >= 0 ? -1 : 1;

                auto x2 = xn + sign * (yn * powerOfTwo);
                auto y2 = yn - sign * (xn * powerOfTwo);
                z += sign * (double)Angles[i];

                powerOfTwo = powerOfTwo / 2;

                xn = x2;
                yn = y2;
            }

            double xf = xn * (double)KProd[(n < KProdLen ? n : KProdLen) - 1];
            double yf = yn * (double)KProd[(n < KProdLen ? n : KProdLen) - 1];
            return { xf, yf };
        }

        template <class T>
        constexpr TCordicVec<T, T> Rotate(T x, T y, Angle a, int32 n = (T::B - 1))
        {
            using ValueT = typename T::ValueT;
            using AngleT = Angle::ValueT;

            ValueT xn = x.Value;
            ValueT yn = y.Value;
            AngleT z = AngleShift(a, -PI).Value;

            int32 sign = 1;
            if (z > HALF_PI.Value && z < PI.Value)
            {
                z -= PI.Value;
                sign = -1;
            }
            else if (z < -HALF_PI.Value && z > -PI.Value)
            {
                z += PI.Value;
                sign = -1;
            }

            for (int32 i = 0; i <= n; ++i)
            {
                int32 sigma = z >= 0 ? -1 : 1;

                auto x2 = xn + sigma * (yn >> i);
                auto y2 = yn - sigma * (xn >> i);
                z += sigma * Angles[i].Value;

                xn = x2;
                yn = y2;
            }

            T xf = T(Q64(xn * sign)) * KProd[(n < KProdLen ? n : KProdLen) - 1];
            T yf = T(Q64(yn * sign)) * KProd[(n < KProdLen ? n : KProdLen) - 1];

            return { xf, yf };
        }

        template <class T>
        constexpr TCordicVec<T, Angle> Vector(T x, T y, int32 n = (T::B - 1))
        {
            using ValueT = typename T::ValueT;
            using AngleT = Angle::ValueT;

            ValueT xn = x.Value;
            ValueT yn = y.Value;
            AngleT z = 0;

            if (xn < 0)
            {
                xn = -xn;
                yn = -yn;
                z += (yn >= 0) ? PI.Value : -PI.Value;
            }

            for (int32 i = 0; i <= n; ++i)
            {
                int32 sign = yn > 0 ? 1 : -1;

                auto x2 = xn + sign * (yn >> i);
                auto y2 = yn - sign * (xn >> i);
                z += sign * Angles[i].Value;

                xn = x2;
                yn = y2;
            }

            T m = T(TFixedQ_T<ValueT>(xn)) * KProd[(n < KProdLen ? n : KProdLen) - 1];
            Angle a = TFixedQ_T<Angle::ValueT>(z);
            return { m, a };
        }

        template <class T>
        constexpr T Dot(T x1, T y1, T x2, T y2)
        {
            auto a = Vector(x1, y1);
            auto b = Rotate(x2, y2, -a.Y);
            return a.X * b.X;
        }
    }
}
