
#pragma once

#include <sol/sol.hpp>
#include <sol/types.hpp>

#include "DLLExport.h"
#include "FeatureECS.h"
#include "FixedVector.h"
#include "PlatformTypes.h"
#include "FixedPoint/FixedPoint.h"

struct lua_State;

namespace sol
{
    class state;
}

namespace Phoenix
{
    namespace Lua
    {
        bool IsFP64(lua_State* L, int32 pos);

        FP64 ToFP64(lua_State* L, int32 pos);

        int32 NewFP64(lua_State* L);

        void PushFP64(lua_State* L, const FP64& v);

        FP64 FP64Parse(lua_State* L, int32 pos);

        FP64 FP64FromString(const char* str);

        int32 FP64ToHex(lua_State* L);

        int32 FP64ToString(lua_State* L);

        int32 FP64ToNumber(lua_State* L);

        int32 FP64Add(lua_State* L);
        int32 FP64Sub(lua_State* L);
        int32 FP64Mul(lua_State* L);
        int32 FP64Div(lua_State* L);
        int32 FP64Mod(lua_State* L);
        int32 FP64Neg(lua_State* L);
        int32 FP64Min(lua_State* L);
        int32 FP64Max(lua_State* L);
        int32 FP64Clamp(lua_State* L);
        int32 FP64Eq(lua_State* L);
        int32 FP64Lt(lua_State* L);
        int32 FP64Le(lua_State* L);
        int32 FP64Gt(lua_State* L);
        int32 FP64Ge(lua_State* L);
        int32 FP64Compare(lua_State* L);
        int32 FP64Sqrt(lua_State* L);
        int32 FP64Ceil(lua_State* L);
        int32 FP64Floor(lua_State* L);
        int32 FP64Abs(lua_State* L);
        int32 FP64Sin(lua_State* L);
        int32 FP64Cos(lua_State* L);
        int32 FP64Tan(lua_State* L);
        int32 FP64ArcSin(lua_State* L);
        int32 FP64ArcCos(lua_State* L);
        int32 FP64ArcTan(lua_State* L);
        int32 FP64ArcTan2(lua_State* L);
        int32 FP64Exp(lua_State* L);
        int32 FP64Log(lua_State* L);
        int32 FP64Log2(lua_State* L);

        int32 LuaOpen_FP64(lua_State* L);

        // template <class Handler, size_t Tb, class T>
        // bool sol_lua_check(sol::types<TFixed<Tb, T>>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
        // {
        //     int absolute_index = lua_absindex(L, index);
        //     bool success = sol::stack::check<T>(L, absolute_index, Forward(handler));
        //     tracking.use(1);
        //     return success;
        // }
        //
        // template <size_t Tb, class T>
        // TFixed<Tb, T> sol_lua_get(sol::types<TFixed<Tb, T>>, lua_State* L, int index, sol::stack::record& tracking)
        // {
        //     int absolute_index = lua_absindex(L, index);
        //     T value = sol::stack::get<T>(L, absolute_index);
        //     tracking.use(1);
        //     return TFixedQ_T<T>(value);
        // }
        //
        // template <size_t Tb, class T>
        // int32 sol_lua_push(lua_State* L, const TFixed<Tb, T>& v)
        // {
        //     return sol::stack::push(L, v.Value);
        // }

        // template <class Handler, size_t Tb, class T>
        // bool sol_lua_check(sol::types<TVec2<TFixed<Tb, T>>>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
        // {
        //     int absolute_index = lua_absindex(L, index);
        //     bool success =
        //         sol::stack::check<T>(L, absolute_index, Forward(handler)) &&
        //         sol::stack::check<T>(L, absolute_index + 1, Forward(handler));
        //     tracking.use(2);
        //     return success;
        // }
        //
        // template <size_t Tb, class T>
        // TVec2<TFixed<Tb, T>> sol_lua_get(sol::types<TVec2<TFixed<Tb, T>>>, lua_State* L, int index, sol::stack::record& tracking) {
        //     int absolute_index = lua_absindex(L, index);
        //     TVec2<TFixed<Tb, T>> vec;
        //     vec.X = sol::stack::get<T>(L, absolute_index);
        //     vec.Y = sol::stack::get<T>(L, absolute_index + 1);
        //     tracking.use(2);
        //     return vec;
        // }
        //
        // template <size_t Tb, class T>
        // int32 sol_lua_push(lua_State* L, const TVec2<TFixed<Tb, T>>& v)
        // {
        //     return sol::stack::push(L, v.X) + sol::stack::push(L, v.Y);
        // }
    }
}

template <class Handler, Phoenix::uint8 Tb, class T>
bool sol_lua_check(sol::types<Phoenix::TFixed<Tb, T>>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
    int absolute_index = lua_absindex(L, index);
    int type = lua_type(L, absolute_index);

    bool success = false;
    if (type == LUA_TNUMBER)
    {
        success = true;
    }

    // bool success = sol::stack::check<T>(L, absolute_index, std::forward<Handler>(handler));
    tracking.use(1);
    return success;
}

template <Phoenix::uint8 Tb, class T>
Phoenix::TFixed<Tb, T> sol_lua_get(sol::types<Phoenix::TFixed<Tb, T>>, lua_State* L, int index, sol::stack::record& tracking)
{
    int absolute_index = lua_absindex(L, index);
    int type = lua_type(L, absolute_index);

    T v = 0;
    if (type == LUA_TNUMBER)
    {
        if (lua_isinteger(L, absolute_index))
        {
            lua_Integer l = lua_tointeger(L, absolute_index);
            v = Phoenix::TFixed<Tb, T>::ConvertToQ(l);
        }
        else
        {
            v = Phoenix::TFixed<Tb, T>::ConvertToQ(lua_tonumber(L, absolute_index));
        }
    }

    tracking.use(1);
    return Phoenix::TFixedQ_T<T>(v);
}

template <Phoenix::uint8 Tb, class T>
int sol_lua_push(lua_State* L, const Phoenix::TFixed<Tb, T>& v)
{
    return sol::stack::push(L, v.Value);
}

template <class Handler>
bool sol_lua_check(sol::types<Phoenix::ECS::EntityId>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
    int absolute_index = lua_absindex(L, index);
    bool success = sol::stack::check<Phoenix::uint32>(L, absolute_index, std::forward<Handler>(handler));
    tracking.use(1);
    return success;
}

Phoenix::ECS::EntityId sol_lua_get(sol::types<Phoenix::ECS::EntityId>, lua_State* L, int index, sol::stack::record& tracking);

template <class Handler>
bool sol_lua_check(sol::types<Phoenix::Action>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
{
    int absolute_index = lua_absindex(L, index);
    bool success = sol::stack::check<Phoenix::uint32>(L, absolute_index, std::forward<Handler>(handler));
    tracking.use(1);
    return success;
}

Phoenix::Action sol_lua_get(sol::types<Phoenix::Action>, lua_State* L, int index, sol::stack::record& tracking);

int sol_lua_push(lua_State* L, const Phoenix::Action& v);