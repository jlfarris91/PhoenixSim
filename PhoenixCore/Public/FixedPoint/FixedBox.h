
#pragma once

namespace Phoenix
{
    template <class TVec>
    struct TFixedBox
    {
        using TVecComp = typename TVec::ComponentT;

        constexpr TFixedBox() {}
        constexpr TFixedBox(const TVec& min, const TVec& max) : Min(min), Max(max) {}
        constexpr TFixedBox(const TFixedBox& other) : Min(other.Min), Max(other.Max) {}
        constexpr TFixedBox(TFixedBox&& other) noexcept : Min(other.Min), Max(other.Max) {}

        static TFixedBox FromPoints(const TVec& a, const TVec& b, const TVec& c)
        {
            TFixedBox box(TVec::Max, TVec::Min);
            box.Union(a);
            box.Union(b);
            box.Union(c);
            return box;
        }

        static TFixedBox FromPoints(TVec* points, size_t num)
        {
            TFixedBox box(TVec::Max, TVec::Min);
            for (size_t i = 0; i < num; ++i)
            {
                box.Union(*(points + i));
            }
            return box;
        }

        constexpr TVec GetCenter() const
        {
            return TVec::Midpoint(Min, Max);
        }

        constexpr auto GetExtents() const
        {
            return (Max - Min) / 2;
        }

        constexpr auto GetSize() const
        {
            return Max - Min;
        }

        constexpr bool Overlaps(const TFixedBox& other) const
        {
            if constexpr (requires { TVec::Z; })
            {
                return Min.X <= other.Max.X && Max.X >= other.Min.X &&
                       Min.Y <= other.Max.Y && Max.Y >= other.Min.Y &&
                       Min.Z <= other.Max.Z && Max.Z >= other.Min.Z;
            }

            return Min.X <= other.Max.X && Max.X >= other.Min.X &&
                   Min.Y <= other.Max.Y && Max.Y >= other.Min.Y;
        }

        constexpr bool Contains(const TVec& v) const
        {
            if constexpr (requires { TVec::Z; })
            {
                return v.X >= Min.X && v.X <= Max.X &&
                       v.Y >= Min.Y && v.Y <= Max.Y &&
                       v.Z >= Min.Z && v.Z <= Max.Z;
            }

            return v.X >= Min.X && v.X <= Max.X && v.Y >= Min.Y && v.Y <= Max.Y;
        }

        constexpr bool Contains(const TFixedBox& other) const
        {
            if constexpr (requires { TVec::Z; })
            {
                return Min.X <= other.Min.X && Max.X >= other.Max.X &&
                       Min.Y <= other.Min.Y && Max.Y >= other.Max.Y &&
                       Min.Z <= other.Min.Z && Max.Z >= other.Max.Z;
            }

            return Min.X <= other.Min.X && Max.X >= other.Max.X &&
                   Min.Y <= other.Min.Y && Max.Y >= other.Max.Y;
        }

        constexpr TFixedBox ExpandBy(const TVec& v) const
        {
            return FixedBox(Min - v, Max + v);
        }

        constexpr TFixedBox ExpandBy(const TVecComp& v) const
        {
            return FixedBox(Min - v, Max + v);
        }

        constexpr TFixedBox& Union(const TVec& pt)
        {
            Min.X = Phoenix::Min(Min.X, pt.X);
            Min.Y = Phoenix::Min(Min.Y, pt.Y);
            Max.X = Phoenix::Max(Max.X, pt.X);
            Max.Y = Phoenix::Max(Max.Y, pt.Y);
            return *this;
        }

        constexpr TFixedBox Union(const TVec& pt) const
        {
            TFixedBox result;
            result.Min.X = Phoenix::Min(Min.X, pt.X);
            result.Min.Y = Phoenix::Min(Min.Y, pt.Y);
            result.Max.X = Phoenix::Max(Max.X, pt.X);
            result.Max.Y = Phoenix::Max(Max.Y, pt.Y);
            return result;
        }

        constexpr TFixedBox Union(const TFixedBox& other) const
        {
            TFixedBox result;
            result.Min.X = Phoenix::Min(Min.X, other.Min.X);
            result.Min.Y = Phoenix::Min(Min.Y, other.Min.Y);
            result.Max.X = Phoenix::Max(Max.X, other.Max.X);
            result.Max.Y = Phoenix::Max(Max.Y, other.Max.Y);
            return result;
        }

        TVec Min = TVec::Zero;
        TVec Max = TVec::Zero;
    };
}