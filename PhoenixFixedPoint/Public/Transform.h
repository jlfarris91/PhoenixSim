
#pragma once

#include "Vector.h"

namespace Phoenix
{
    template <class TVec, class TRot, class TScale>
    struct TTransform
    {
        TVec Position = TZero<TVec>::Value;
        TRot Rotation = TZero<TRot>::Value;
        TScale Scale = TOne<TScale>::Value;
        TVec RotateVector(const TVec& vec);
    };

    typedef TTransform<Vec2, Angle, Value> Transform2D;

    template <>
    inline Vec2 TTransform<Vec2, Angle, Value>::RotateVector(const Vec2& vec)
    {
        return vec.Rotate(Rotation);
    }
}