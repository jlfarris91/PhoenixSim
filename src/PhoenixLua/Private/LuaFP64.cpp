
#include "LuaFP64.h"

#include <sol/state.hpp>

using namespace Phoenix;

bool Lua::IsFP64(lua_State* L, int32 pos)
{
    if (lua_getmetatable(L, pos))
    {
        luaL_getmetatable(L, "FP64");
        int32 equal = lua_rawequal(L, -1, -2);
        lua_pop(L, 2);
        return equal;
    }
    return false;
}

FP64 Lua::ToFP64(lua_State* L, int32 pos)
{
    FP64 v;
    int32 type = lua_type(L, pos);

    switch (type)
    {
        case LUA_TNUMBER:
            {
                if (lua_isinteger(L, pos))
                {
                    v = FP64(lua_tointeger(L, pos));
                }
                else
                {
                    v = FP64(static_cast<FP64::TValue>(lua_tonumber(L, pos)));
                }
                break;
            }
        case LUA_TUSERDATA:
            {
                if (IsFP64(L, pos))
                {
                    v = *(FP64*)lua_touserdata(L, pos);
                }
                break;
            }
        default:
            {
                const char *msg = lua_pushfstring(L, "%s expected, got %s", "FP64", luaL_typename(L, pos));
                return luaL_argerror(L, pos, msg);
            }
    }

    return v;
}

int32 Lua::NewFP64(lua_State* L)
{
    // TODO (jfarris): How do we get the 'B' ?

    int32 type = lua_type(L, 1);
    uint8 b = static_cast<uint8>(lua_tointeger(L, 1));

    FP64 v(0, b);

    if (type == LUA_TSTRING)
    {
        v = FP64Parse(L, 1);
    }
    else if (type == LUA_TNUMBER)
    {
        if (lua_isinteger(L, 1))
        {
            v = FP64(lua_tointeger(L, 1));
        }
        else
        {
            v = FP64(static_cast<FP64::TValue>(lua_tonumber(L, 1)));
        }
    }

    PushFP64(L, v);
    return 1;
}

void Lua::PushFP64(lua_State* L, const FP64& v)
{
    FP64* p = (FP64*)lua_newuserdata(L, sizeof(FP64));
    *p = v;
    luaL_getmetatable(L, "FP64");
    lua_setmetatable(L, -2);
}

FP64 Lua::FP64Parse(lua_State* L, int32 pos)
{
    const char* str = lua_tostring(L, pos);
    return FP64FromString(str);
}

FP64 Lua::FP64FromString(const char* str)
{
    // TODO (jfarris): 
    return FP64();
}

int32 Lua::FP64ToHex(lua_State* L)
{
    return 1;
}

int32 Lua::FP64ToString(lua_State* L)
{
    return 1;
}

int32 Lua::FP64ToNumber(lua_State* L)
{
    return 1;
}

int32 Lua::FP64Add(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, lhs + rhs);
    return 1;
}

int32 Lua::FP64Sub(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, lhs - rhs);
    return 1;
}

int32 Lua::FP64Mul(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, lhs * rhs);
    return 1;
}

int32 Lua::FP64Div(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    if (rhs.Value == 0)
    {
        return luaL_error(L, "Divide by zero!");
    }
    // PushFP64(L, lhs / rhs);
    return 1;
}

int32 Lua::FP64Mod(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    if (rhs.Value == 0)
    {
        return luaL_error(L, "Mod by zero!");
    }
    // PushFP64(L, lhs % rhs);
    return 1;
}

int32 Lua::FP64Neg(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    // PushFP64(L, -lhs);
    return 1;
}

int32 Lua::FP64Min(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, Min(lhs, rhs));
    return 1;
}

int32 Lua::FP64Max(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, Max(lhs, rhs));
    return 1;
}

int32 Lua::FP64Clamp(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    FP64 min = ToFP64(L, 2);
    FP64 max = ToFP64(L, 3);
    // PushFP64(L, Clamp(x, min, max));
    return 1;
}

int32 Lua::FP64Eq(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, lhs == rhs);
    return 1;
}

int32 Lua::FP64Lt(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, lhs < rhs);
    return 1;
}

int32 Lua::FP64Le(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, lhs <= rhs);
    return 1;
}

int32 Lua::FP64Gt(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, lhs > rhs);
    return 1;
}

int32 Lua::FP64Ge(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, lhs >= rhs);
    return 1;
}

int32 Lua::FP64Compare(lua_State* L)
{
    FP64 lhs = ToFP64(L, 1);
    FP64 rhs = ToFP64(L, 2);
    // PushFP64(L, (lhs == rhs ? 0 : lhs < rhs ? -1 : 1));
    return 1;
}

int32 Lua::FP64Sqrt(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    // PushFP64(L, Sqrt(x));
    return 1;
}

int32 Lua::FP64Ceil(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    // PushFP64(L, Ceil(x));
    return 1;
}

int32 Lua::FP64Floor(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    // PushFP64(L, Floor(x));
    return 1;
}

int32 Lua::FP64Abs(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    // PushFP64(L, Abs(x));
    return 1;
}

int32 Lua::FP64Sin(lua_State* L)
{
    FP64 angle = ToFP64(L, 1);
    // PushFP64(L, Sin(angle));
    return 1;
}

int32 Lua::FP64Cos(lua_State* L)
{
    FP64 angle = ToFP64(L, 1);
    // PushFP64(L, Cos(angle));
    return 1;
}

int32 Lua::FP64Tan(lua_State* L)
{
    FP64 angle = ToFP64(L, 1);
    // PushFP64(L, Tan(angle));
    return 1;
}

int32 Lua::FP64ArcSin(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    // PushFP64(L, ArcSin(x));
    return 1;
}

int32 Lua::FP64ArcCos(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    // PushFP64(L, ArcCos(x));
    return 1;
}

int32 Lua::FP64ArcTan(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    // PushFP64(L, ArcTan(x));
    return 1;
}

int32 Lua::FP64ArcTan2(lua_State* L)
{
    FP64 y = ToFP64(L, 1);
    FP64 x = ToFP64(L, 2);
    // PushFP64(L, ArcTan2(y, x));
    return 1;
}

int32 Lua::FP64Exp(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    // PushFP64(L, Exp(x));
    return 1;
}

int32 Lua::FP64Log(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    // PushFP64(L, Log(x));
    return 1;
}

int32 Lua::FP64Log2(lua_State* L)
{
    FP64 x = ToFP64(L, 1);
    // PushFP64(L, Log2(x));
    return 1;
}

int32 Lua::LuaOpen_FP64(lua_State* L)
{
    static const luaL_Reg libFP64_meta [] = {
        {"__add", FP64Add},
        {"__sub", FP64Sub},
        {"__mul", FP64Mul},
        {"__div", FP64Div},
        {"__mod", FP64Mod},
        {"__unm", FP64Neg},
        {"__eq", FP64Eq},
        {"__lt", FP64Lt},
        {"__le", FP64Le},
        {"__gt", FP64Gt},
        {"__ge", FP64Ge},
        {"__tostring", FP64ToString},
        {"__index", NULL},
        {NULL, NULL}
    };

    static const luaL_Reg libFP64 [] = {
        {"hex", FP64ToHex},
        {"tostring", FP64ToString},
        {"tonumber", FP64ToNumber},
        {"compare", FP64Compare},
        {"min", FP64Min},
        {"max", FP64Max},
        {"clamp", FP64Clamp},
        {"equals", FP64Eq},
        {"sqrt", FP64Sqrt},
        {"ceil", FP64Ceil},
        {"floor", FP64Floor},
        {"abs", FP64Abs},
        {"sin", FP64Sin},
        {"cos", FP64Cos},
        {"tan", FP64Tan},
        {"asin", FP64ArcSin},
        {"acos", FP64ArcCos},
        {"atan", FP64ArcTan},
        {"atan2", FP64ArcTan2},
        {"exp", FP64Exp},
        {"log", FP64Log},
        {"log2", FP64Log2},
        {"new", NewFP64},
        {"one", NULL},
        {"pi", NULL},
        {"epsilon", NULL},
        {"rad2deg", NULL},
        {"deg2rad", NULL},
        {"e", NULL},
        {NULL, NULL}
    };

    luaL_newmetatable(L, "FP64");
    luaL_setfuncs(L, libFP64_meta, 0);
    luaL_newlib(L, libFP64);

    sol::state_view lua(L);

    // lua.new_usertype<Distance>("Distance",
    //     sol::constructors<Distance(), Distance(double), Distance(int32)>(),
    //     "v", &Distance::Value);

    return 1;
}

ECS::EntityId sol_lua_get(sol::types<ECS::EntityId>, lua_State* L, int index, sol::stack::record& tracking)
{
    int absolute_index = lua_absindex(L, index);
    uint32 value = sol::stack::get<uint32>(L, absolute_index);
    tracking.use(1);
    return ECS::EntityId(value);
}

Action sol_lua_get(sol::types<Action>, lua_State* L, int index, sol::stack::record& tracking)
{
    int absolute_index = lua_absindex(L, index);
    int count = 0;

    Action action;
    action.Verb = sol::stack::get<FName>(L, absolute_index + count++);
    action.Sender = sol::stack::get<FName>(L, absolute_index + count++);
    for (Data& data : action.Data)
    {
        data = sol::stack::get<Data>(L, absolute_index + count++);
    }

    tracking.use(count);
    return action;
}

int sol_lua_push(lua_State* L, const Action& v)
{
    int count = 0;
    count += sol::stack::push(L, v.Verb);
    count += sol::stack::push(L, v.Sender);
    for (const Data& data : v.Data)
    {
        count += sol::stack::push(L, data);
    }
    return count;
}
