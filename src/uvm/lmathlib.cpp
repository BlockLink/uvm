/*
** $Id: lmathlib.c,v 1.117 2015/10/02 15:39:23 roberto Exp $
** Standard mathematical library
** See Copyright Notice in lua.h
*/

#define lmathlib_cpp

#include "uvm/lprefix.h"


#include <stdlib.h>
#include <math.h>

#include "uvm/lua.h"

#include "uvm/lauxlib.h"
#include "uvm/lualib.h"


#undef PI
#define PI	(l_mathop(3.141592653589793238462643383279502884))
#undef PI_STR
#define PI_STR "3.141592653589793238462643383279502884"

static int math_abs(lua_State *L) {
    if (lua_isinteger(L, 1)) {
        lua_Integer n = lua_tointeger(L, 1);
        if (n < 0) n = (lua_Integer)(0u - (lua_Unsigned)n);
        lua_pushinteger(L, n);
    }
	else {
		auto n = luaL_checknumber(L, 1);
		auto r = n.sign ? n : safe_number_neg(n);
		lua_pushnumber(L, r);
	}
    return 1;
}

static int math_toint(lua_State *L) {
    int valid;
    lua_Integer n = lua_tointegerx(L, 1, &valid);
    if (valid)
        lua_pushinteger(L, n);
    else {
        luaL_checkany(L, 1);
        lua_pushnil(L);  /* value is not convertible to integer */
    }
    return 1;
}


static void pushnumint(lua_State *L, lua_Number d) {
    lua_Integer n;
    if (lua_numbertointeger(d, &n))  /* does 'd' fit in an integer? */
        lua_pushinteger(L, n);  /* result is integer */
    else
        lua_pushnumber(L, d);  /* result is float */
}


static int math_floor(lua_State *L) {
    if (lua_isinteger(L, 1))
        lua_settop(L, 1);  /* integer is its own floor */
    else {
        auto d = safe_number_to_int64(luaL_checknumber(L, 1));
		lua_pushinteger(L, d);
    }
    return 1;
}


static int math_ceil(lua_State *L) {
    if (lua_isinteger(L, 1))
        lua_settop(L, 1);  /* integer is its own ceil */
    else {
		const auto& n = luaL_checknumber(L, 1);
		auto d = safe_number_to_int64(n);
		if (safe_number_lt(safe_number_create(d), n))
			++d;
		lua_pushinteger(L, d);
    }
    return 1;
}


static int math_fmod(lua_State *L) {
	if (lua_isinteger(L, 1) && lua_isinteger(L, 2)) {
		lua_Integer d = lua_tointeger(L, 2);
		if ((lua_Unsigned)d + 1u <= 1u) {  /* special cases: -1 or 0 */
			luaL_argcheck(L, d != 0, 2, "zero");
			lua_pushinteger(L, 0);  /* avoid overflow with 0x80000... / -1 */
		}
		else
			lua_pushinteger(L, lua_tointeger(L, 1) % d);
	}
	else {
		const auto& arg1 = luaL_checknumber(L, 1);
		const auto& arg2 = luaL_checknumber(L, 2);
		const auto& mod_result = safe_number_mod(arg1, arg2);
		lua_pushnumber(L, mod_result);
		luaL_checknumber(L, 2);
	}
    return 1;
}


static int math_sqrt(lua_State *L) {
	const auto& n = luaL_checknumber(L, 1);
	auto nv = std::stod(safe_number_to_string(n)); // valid here
	auto sqrt_v = l_mathop(sqrt)(nv);
	auto sqrt_sn = safe_number_create(std::floor(sqrt_v));
	if (safe_number_gt(safe_number_multiply(sqrt_sn, sqrt_sn), n))
		sqrt_sn = safe_number_minus(sqrt_sn, safe_number_create(1));
    lua_pushnumber(L, sqrt_sn);
    return 1;
}


static int math_ult(lua_State *L) {
    lua_Integer a = luaL_checkinteger(L, 1);
    lua_Integer b = luaL_checkinteger(L, 2);
    lua_pushboolean(L, (lua_Unsigned)a < (lua_Unsigned)b);
    return 1;
}

static int math_min(lua_State *L) {
    int n = lua_gettop(L);  /* number of arguments */
    int imin = 1;  /* index of current minimum value */
    int i;
    luaL_argcheck(L, n >= 1, 1, "value expected");
    for (i = 2; i <= n; i++) {
        if (lua_compare(L, i, imin, LUA_OPLT))
            imin = i;
    }
    lua_pushvalue(L, imin);
    return 1;
}


static int math_max(lua_State *L) {
    int n = lua_gettop(L);  /* number of arguments */
    int imax = 1;  /* index of current maximum value */
    int i;
    luaL_argcheck(L, n >= 1, 1, "value expected");
    for (i = 2; i <= n; i++) {
        if (lua_compare(L, imax, i, LUA_OPLT))
            imax = i;
    }
    lua_pushvalue(L, imax);
    return 1;
}

static int math_type(lua_State *L) {
    if (lua_type(L, 1) == LUA_TNUMBER) {
        if (lua_isinteger(L, 1))
            lua_pushliteral(L, "integer");
        else
            lua_pushliteral(L, "float");
    }
    else {
        luaL_checkany(L, 1);
        lua_pushnil(L);
    }
    return 1;
}


static const luaL_Reg mathlib[] = {
    { "abs", math_abs },
    // { "acos", math_acos },
    // { "asin", math_asin },
    // { "atan", math_atan },
    // { "ceil", math_ceil },
    // { "cos", math_cos },
    // { "deg", math_deg },
    // { "exp", math_exp },
    { "tointeger", math_toint },
    { "floor", math_floor },
    // { "fmod", math_fmod },
    // { "ult", math_ult },
    // { "log", math_log },
    { "max", math_max },
    { "min", math_min },
    // { "modf", math_modf },
    // { "rad", math_rad },
    // { "sin", math_sin },
    // { "sqrt", math_sqrt },
    // { "tan", math_tan },
    { "type", math_type },
    /* placeholders */
    { "pi", nullptr },
    // { "huge", nullptr },
    { "maxinteger", nullptr },
    { "mininteger", nullptr },
    { nullptr, nullptr }
};


/*
** Open math library
*/
LUAMOD_API int luaopen_math(lua_State *L) {
    luaL_newlib(L, mathlib);
    lua_pushnumber(L, safe_number_create(PI_STR));
    lua_setfield(L, -2, "pi");
    // lua_pushnumber(L, (lua_Number)HUGE_VAL);
    // lua_setfield(L, -2, "huge");
    lua_pushinteger(L, LUA_MAXINTEGER);
    lua_setfield(L, -2, "maxinteger");
    lua_pushinteger(L, LUA_MININTEGER);
    lua_setfield(L, -2, "mininteger");
    return 1;
}

