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

static int math_abs(lua_State *L) {
    if (lua_isinteger(L, 1)) {
        lua_Integer n = lua_tointeger(L, 1);
        if (n < 0) n = (lua_Integer)(0u - (lua_Unsigned)n);
        lua_pushinteger(L, n);
    }
	else {
		auto n = luaL_checknumber(L, 1);
		auto n_value = std::stod(safe_number_to_string(n));
		lua_pushnumber(L, l_mathop(fabs)(n_value));
	}
    return 1;
}

static int math_sin(lua_State *L) {
	auto n = luaL_checknumber(L, 1);
	auto n_value = std::stod(safe_number_to_string(n));
    lua_pushnumber(L, l_mathop(sin)(n_value));
    return 1;
}

static int math_cos(lua_State *L) {
	auto n = luaL_checknumber(L, 1);
	auto n_value = std::stod(safe_number_to_string(n));
    lua_pushnumber(L, l_mathop(cos)(n_value));
    return 1;
}

static int math_tan(lua_State *L) {
	auto n = luaL_checknumber(L, 1);
	auto n_value = std::stod(safe_number_to_string(n));
    lua_pushnumber(L, l_mathop(tan)(n_value));
    return 1;
}

static int math_asin(lua_State *L) {
	auto n = luaL_checknumber(L, 1);
	auto n_value = std::stod(safe_number_to_string(n));
    lua_pushnumber(L, l_mathop(asin)(n_value));
    return 1;
}

static int math_acos(lua_State *L) {
	auto n = luaL_checknumber(L, 1);
	auto n_value = std::stod(safe_number_to_string(n));
    lua_pushnumber(L, l_mathop(acos)(n_value));
    return 1;
}

static int math_atan(lua_State *L) {
    lua_Number y = luaL_checknumber(L, 1);
    lua_Number x = luaL_optnumber(L, 2, safe_number_create(1));
	auto y_value = std::stod(safe_number_to_string(y));
	auto x_value = std::stod(safe_number_to_string(x));
    lua_pushnumber(L, l_mathop(atan2)(y_value, x_value));
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
        auto d = l_mathop(floor)(std::stod(safe_number_to_string(luaL_checknumber(L, 1))));
        pushnumint(L, safe_number_create(std::to_string(d)));
    }
    return 1;
}


static int math_ceil(lua_State *L) {
    if (lua_isinteger(L, 1))
        lua_settop(L, 1);  /* integer is its own ceil */
    else {
        auto d = l_mathop(ceil)(std::stod(safe_number_to_string(luaL_checknumber(L, 1))));
        pushnumint(L, safe_number_create(std::to_string(d)));
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
		auto arg1_v = std::stod(safe_number_to_string(arg1));
		auto arg2_v = std::stod(safe_number_to_string(arg2));
		auto fmod_v = l_mathop(fmod)(arg1_v, arg2_v);
		lua_pushnumber(L, fmod_v);
		luaL_checknumber(L, 2);
	}
    return 1;
}


/*
** next function does not use 'modf', avoiding problems with 'double*'
** (which is not compatible with 'float*') when lua_Number is not
** 'double'.
*/
static int math_modf(lua_State *L) {
    if (lua_isinteger(L, 1)) {
        lua_settop(L, 1);  /* number is its own integer part */
        lua_pushnumber(L, 0);  /* no fractional part */
    }
    else {
        lua_Number n = luaL_checknumber(L, 1);
		auto nv = std::stod(safe_number_to_string(n));
        /* integer part (rounds toward zero) */
        lua_Number ip = safe_number_create(std::to_string((safe_number_lt(n, safe_number_zero())) ? l_mathop(ceil)(nv) : l_mathop(floor)(nv)));
        pushnumint(L, ip);
        /* fractional part (test needed for inf/-inf) */
        lua_pushnumber(L, (safe_number_eq(n, ip)) ? safe_number_zero() : safe_number_minus(n, ip));
    }
    return 2;
}


static int math_sqrt(lua_State *L) {
	const auto& n = luaL_checknumber(L, 1);
	auto nv = std::stod(safe_number_to_string(n));
    lua_pushnumber(L, l_mathop(sqrt)(nv));
    return 1;
}


static int math_ult(lua_State *L) {
    lua_Integer a = luaL_checkinteger(L, 1);
    lua_Integer b = luaL_checkinteger(L, 2);
    lua_pushboolean(L, (lua_Unsigned)a < (lua_Unsigned)b);
    return 1;
}

static int math_log(lua_State *L) {
    lua_Number x = luaL_checknumber(L, 1);
    lua_Number res;
    if (lua_isnoneornil(L, 2))
        res = safe_number_create(l_mathop(log)(std::stod(safe_number_to_string(x))));
    else {
        lua_Number base = luaL_checknumber(L, 2);
#if !defined(LUA_USE_C89)
        if (base == 2.0) res = l_mathop(log2)(x); else
#endif
            if (safe_number_eq(base, safe_number_create(10))) res = safe_number_create(std::to_string(l_mathop(log10)(std::stod(safe_number_to_string(x)))));
            else res = safe_number_create(std::to_string(l_mathop(log)(std::stod(safe_number_to_string(x))) / l_mathop(log)(std::stod(safe_number_to_string(base)))));
    }
    lua_pushnumber(L, res);
    return 1;
}

static int math_exp(lua_State *L) {
    lua_pushnumber(L, l_mathop(exp)(std::stod(safe_number_to_string(luaL_checknumber(L, 1)))));
    return 1;
}

static int math_deg(lua_State *L) {
	const auto& n = luaL_checknumber(L, 1);
	auto nd = std::stod(safe_number_to_string(n));
    lua_pushnumber(L, safe_number_create(nd * (l_mathop(180.0) / PI)));
    return 1;
}

static int math_rad(lua_State *L) {
	const auto& n = luaL_checknumber(L, 1);
	auto nd = std::stod(safe_number_to_string(n));
    lua_pushnumber(L, safe_number_create(nd * (PI / l_mathop(180.0))));
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


/*
** {==================================================================
** Deprecated functions (for compatibility only)
** ===================================================================
*/
#if defined(LUA_COMPAT_MATHLIB)

static int math_cosh(lua_State *L) {
    lua_pushnumber(L, l_mathop(cosh)(luaL_checknumber(L, 1)));
    return 1;
}

static int math_sinh(lua_State *L) {
    lua_pushnumber(L, l_mathop(sinh)(luaL_checknumber(L, 1)));
    return 1;
}

static int math_tanh(lua_State *L) {
    lua_pushnumber(L, l_mathop(tanh)(luaL_checknumber(L, 1)));
    return 1;
}

static int math_pow(lua_State *L) {
    lua_Number x = luaL_checknumber(L, 1);
    lua_Number y = luaL_checknumber(L, 2);
    lua_pushnumber(L, l_mathop(pow)(x, y));
    return 1;
}

static int math_frexp(lua_State *L) {
    int e;
    lua_pushnumber(L, l_mathop(frexp)(luaL_checknumber(L, 1), &e));
    lua_pushinteger(L, e);
    return 2;
}

static int math_ldexp(lua_State *L) {
    lua_Number x = luaL_checknumber(L, 1);
    int ep = (int)luaL_checkinteger(L, 2);
    lua_pushnumber(L, l_mathop(ldexp)(x, ep));
    return 1;
}

static int math_log10(lua_State *L) {
    lua_pushnumber(L, l_mathop(log10)(luaL_checknumber(L, 1)));
    return 1;
}

#endif
/* }================================================================== */



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
    lua_pushnumber(L, PI);
    lua_setfield(L, -2, "pi");
    // lua_pushnumber(L, (lua_Number)HUGE_VAL);
    // lua_setfield(L, -2, "huge");
    lua_pushinteger(L, LUA_MAXINTEGER);
    lua_setfield(L, -2, "maxinteger");
    lua_pushinteger(L, LUA_MININTEGER);
    lua_setfield(L, -2, "mininteger");
    return 1;
}

