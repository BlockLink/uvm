#define lsafemathlib_cpp

#include "uvm/lprefix.h"


#include <stdlib.h>
#include <math.h>

#include "uvm/lua.h"

#include "uvm/lauxlib.h"
#include "uvm/lualib.h"
#include <boost/algorithm/hex.hpp>
#include <boost/multiprecision/cpp_int.hpp>

// bigint stores as { hex: hex string, type: 'bigint' }


static void push_bigint(lua_State *L, boost::multiprecision::int256_t value) {
	auto hex_str = boost::algorithm::hex(value.str());
	lua_newtable(L);
	lua_pushstring(L, hex_str.c_str());
	lua_setfield(L, -2, "hex");
	lua_pushstring(L, "bigint");
	lua_setfield(L, -2, "type");
}

static int safemath_bigint(lua_State *L) {
	if (lua_gettop(L) < 1) {
		luaL_argcheck(L, false, 1, "argument is empty");
		return 0;
	}
	if (lua_isinteger(L, 1)) {
		lua_Integer n = lua_tointeger(L, 1);
		auto value_hex = boost::algorithm::hex(std::to_string(n));
		lua_newtable(L);
		lua_pushstring(L, value_hex.c_str());
		lua_setfield(L, -2, "hex");
		lua_pushstring(L, "bigint");
		lua_setfield(L, -2, "type");
		return 1;
	}
	else if (lua_isstring(L, 1)) {
		std::string int_str = luaL_checkstring(L, 1);
		std::string value_hex;
		try {
			value_hex = boost::algorithm::hex(int_str);
		}
		catch (const std::exception& e) {
			luaL_error(L, "invalid int string");
			return 0;
		}
		lua_newtable(L);
		lua_pushstring(L, value_hex.c_str());
		lua_setfield(L, -2, "hex");
		lua_pushstring(L, "bigint");
		lua_setfield(L, -2, "type");
		return 1;
	}
	else {
		luaL_argcheck(L, false, 1, "first argument must be integer or hex string");
		return 0;
	}
	return 1;
}

static bool is_valid_bigint_obj(lua_State *L, int index, std::string& out)
{
	if ((index > 0 && lua_gettop(L) < index) || (index < 0 && lua_gettop(L) < -index) || index == 0) {
		return false;
	}
	if (!lua_istable(L, index)) {
		return false;
	}
	lua_getfield(L, index, "type");
	if (lua_isnil(L, -1) || !lua_isstring(L, -1) || strcmp("bigint", luaL_checkstring(L, -1)) != 0) {
		lua_pop(L, 1);
		return false;
	}
	lua_pop(L, 1);
	lua_getfield(L, index, "hex");
	if (lua_isnil(L, -1) || !lua_isstring(L, -1) || strlen(luaL_checkstring(L, -1)) < 1) {
		lua_pop(L, 1);
		return false;
	}
	std::string hex_str = luaL_checkstring(L, -1);
	lua_pop(L, 1);
	try {
		boost::algorithm::unhex(hex_str);
		out = hex_str;
		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

static bool is_same_direction_int256(boost::multiprecision::int256_t a, boost::multiprecision::int256_t b)
{
	return (a > 0 && b > 0) || (a < 0 && b < 0);
}

static bool is_same_direction_int512(boost::multiprecision::int512_t a, boost::multiprecision::int512_t b)
{
	return (a > 0 && b > 0) || (a < 0 && b < 0);
}

static int safemath_add(lua_State *L) {
	if (lua_gettop(L) < 2) {
		luaL_error(L, "add need at least 2 argument");
	}
	std::string first_hex_str;
	std::string second_hex_str;
	if (!is_valid_bigint_obj(L, 1, first_hex_str)) {
		luaL_argcheck(L, false, 1, "invalid bigint obj");
	}
	if (!is_valid_bigint_obj(L, 2, second_hex_str)) {
		luaL_argcheck(L, false, 2, "invalid bigint obj");
	}
	auto first_int_str = boost::algorithm::unhex(first_hex_str);
	boost::multiprecision::int256_t first_int(first_int_str);
	auto second_int_str = boost::algorithm::unhex(second_hex_str);
	boost::multiprecision::int256_t second_int(second_int_str);
	auto result_int = first_int + second_int;
	// overflow check
	if (is_same_direction_int256(first_int, second_int)) {
		if ((first_int > 0 && result_int <= 0) || (first_int<0 && result_int >= 0)) {
			luaL_error(L, "int256 overflow");
		}
	}
	push_bigint(L, result_int);
	return 1;
}

static int safemath_mul(lua_State *L) {
	if (lua_gettop(L) < 2) {
		luaL_error(L, "mul need at least 2 argument");
	}
	std::string first_hex_str;
	std::string second_hex_str;
	if (!is_valid_bigint_obj(L, 1, first_hex_str)) {
		luaL_argcheck(L, false, 1, "invalid bigint obj");
	}
	if (!is_valid_bigint_obj(L, 2, second_hex_str)) {
		luaL_argcheck(L, false, 2, "invalid bigint obj");
	}
	auto first_int_str = boost::algorithm::unhex(first_hex_str);
	boost::multiprecision::int256_t first_int(first_int_str);
	auto second_int_str = boost::algorithm::unhex(second_hex_str);
	boost::multiprecision::int256_t second_int(second_int_str);
	auto result_int = boost::multiprecision::int512_t(first_int) * boost::multiprecision::int512_t(second_int);
	// overflow check
	boost::multiprecision::int256_t int256_max("57896044618658097711785492504343953926634992332820282019728792003956564819968");
	boost::multiprecision::int256_t int256_min("-115792089237316195423570985008687907853269984665640564039457584007913129639935");
	if (is_same_direction_int256(first_int, second_int) && result_int > int256_max) {
		luaL_error(L, "int256 overflow");
	}
	else if (is_same_direction_int256(first_int, second_int) && result_int < int256_min) {
		luaL_error(L, "int256 overflow");
	}
	push_bigint(L, result_int.convert_to<boost::multiprecision::int256_t>());
	return 1;
}

static boost::multiprecision::int256_t int256_pow(lua_State *L, boost::multiprecision::int256_t value, boost::multiprecision::int256_t n) {
	if (n > 100) {
		luaL_error(L, "too large value in bigint pow");
	}
	boost::multiprecision::int512_t result(value);
	boost::multiprecision::int256_t int256_max("57896044618658097711785492504343953926634992332820282019728792003956564819968");
	boost::multiprecision::int256_t int256_min("-115792089237316195423570985008687907853269984665640564039457584007913129639935");
	for (int i = 1; i < n; i++) {
		auto mid_value = result * value;
		// overflow check
		if (is_same_direction_int512(result, value) && mid_value > int256_max) {
			luaL_error(L, "int256 overflow");
		}
		else if(!is_same_direction_int512(result, value) && mid_value < int256_min) {
			luaL_error(L, "int256 overflow");
		}
		result = mid_value;
	}
	return result.convert_to<boost::multiprecision::int256_t>();
}

static int safemath_pow(lua_State *L) {
	if (lua_gettop(L) < 2) {
		luaL_error(L, "pow need at least 2 argument");
	}
	std::string first_hex_str;
	std::string second_hex_str;
	if (!is_valid_bigint_obj(L, 1, first_hex_str)) {
		luaL_argcheck(L, false, 1, "invalid bigint obj");
	}
	if (!is_valid_bigint_obj(L, 2, second_hex_str)) {
		luaL_argcheck(L, false, 2, "invalid bigint obj");
	}
	auto first_int_str = boost::algorithm::unhex(first_hex_str);
	boost::multiprecision::int256_t first_int(first_int_str);
	auto second_int_str = boost::algorithm::unhex(second_hex_str);
	boost::multiprecision::int256_t second_int(second_int_str);
	auto result_int = int256_pow(L, first_int, second_int);
	// overflow check
	if (result_int <= 0) {
		luaL_error(L, "int256 overflow");
	}
	push_bigint(L, result_int);
	return 1;
}

static int safemath_div(lua_State *L) {
	if (lua_gettop(L) < 2) {
		luaL_error(L, "div need at least 2 argument");
	}
	std::string first_hex_str;
	std::string second_hex_str;
	if (!is_valid_bigint_obj(L, 1, first_hex_str)) {
		luaL_argcheck(L, false, 1, "invalid bigint obj");
	}
	if (!is_valid_bigint_obj(L, 2, second_hex_str)) {
		luaL_argcheck(L, false, 2, "invalid bigint obj");
	}
	auto first_int_str = boost::algorithm::unhex(first_hex_str);
	boost::multiprecision::int256_t first_int(first_int_str);
	auto second_int_str = boost::algorithm::unhex(second_hex_str);
	boost::multiprecision::int256_t second_int(second_int_str);
	if (second_int == 0) {
		luaL_error(L, "div by 0 error");
	}
	auto result_int = first_int / second_int;
	// overflow check
	if (is_same_direction_int256(first_int, second_int)) {
		if ((first_int > 0 && result_int <= 0) || (first_int<0 && result_int >= 0)) {
			luaL_error(L, "int256 overflow");
		}
	}
	push_bigint(L, result_int);
	return 1;
}

static int safemath_rem(lua_State *L) {
	if (lua_gettop(L) < 2) {
		luaL_error(L, "rem need at least 2 argument");
	}
	std::string first_hex_str;
	std::string second_hex_str;
	if (!is_valid_bigint_obj(L, 1, first_hex_str)) {
		luaL_argcheck(L, false, 1, "invalid bigint obj");
	}
	if (!is_valid_bigint_obj(L, 2, second_hex_str)) {
		luaL_argcheck(L, false, 2, "invalid bigint obj");
	}
	auto first_int_str = boost::algorithm::unhex(first_hex_str);
	boost::multiprecision::int256_t first_int(first_int_str);
	auto second_int_str = boost::algorithm::unhex(second_hex_str);
	boost::multiprecision::int256_t second_int(second_int_str);
	if (second_int == 0) {
		luaL_error(L, "rem by 0 error");
	}
	auto result_int = first_int % second_int;
	// overflow check
	if (is_same_direction_int256(first_int, second_int)) {
		if ((first_int > 0 && result_int <= 0) || (first_int<0 && result_int >= 0)) {
			luaL_error(L, "int256 overflow");
		}
	}
	push_bigint(L, result_int);
	return 1;
}

static int safemath_sub(lua_State *L) {
	if (lua_gettop(L) < 2) {
		luaL_error(L, "sub need at least 2 argument");
	}
	std::string first_hex_str;
	std::string second_hex_str;
	if (!is_valid_bigint_obj(L, 1, first_hex_str)) {
		luaL_argcheck(L, false, 1, "invalid bigint obj");
	}
	if (!is_valid_bigint_obj(L, 2, second_hex_str)) {
		luaL_argcheck(L, false, 2, "invalid bigint obj");
	}
	auto first_int_str = boost::algorithm::unhex(first_hex_str);
	boost::multiprecision::int256_t first_int(first_int_str);
	auto second_int_str = boost::algorithm::unhex(second_hex_str);
	boost::multiprecision::int256_t second_int(second_int_str);
	auto result_int = first_int - second_int;
	// overflow check
	if (!is_same_direction_int256(first_int, second_int)) {
		if ((first_int > 0 && result_int <= 0) || (first_int<0 && result_int >= 0)) {
			luaL_error(L, "int256 overflow");
		}
	}
	push_bigint(L, result_int);
	return 1;
}

static int safemath_toint(lua_State* L) {
	std::string hex_str;
	if (!is_valid_bigint_obj(L, 1, hex_str)) {
		luaL_argcheck(L, false, 1, "invalid bigint object");
	}
	auto value_str = boost::algorithm::unhex(hex_str);
	boost::multiprecision::int256_t bigint_value(value_str);
	auto value = bigint_value.convert_to<lua_Integer>();
	lua_pushinteger(L, value);
	return 1;
}

static int safemath_tohex(lua_State* L) {
	std::string hex_str;
	if (!is_valid_bigint_obj(L, 1, hex_str)) {
		luaL_argcheck(L, false, 1, "invalid bigint object");
	}
	lua_pushstring(L, hex_str.c_str());
	return 1;
}

static int safemath_tostring(lua_State* L) {
	std::string hex_str;
	if (!is_valid_bigint_obj(L, 1, hex_str)) {
		luaL_argcheck(L, false, 1, "invalid bigint object");
	}
	auto value_str = boost::algorithm::unhex(hex_str);
	boost::multiprecision::int256_t bigint_value(value_str);
	auto bigint_value_str = bigint_value.str();
	lua_pushstring(L, bigint_value_str.c_str());
	return 1;
}

static int safemath_min(lua_State *L) {
	int n = lua_gettop(L);  /* number of arguments */
	int imin = 1;  /* index of current minimum value */
	std::string first_hex_str;
	if (!is_valid_bigint_obj(L, 1, first_hex_str)) {
		luaL_argcheck(L, false, 1, "bigint value expected");
	}
	auto first_int_str = boost::algorithm::unhex(first_hex_str);
	boost::multiprecision::int256_t min_value(first_int_str);
	luaL_argcheck(L, n >= 1, 1, "value expected");
	for (int i = 2; i <= n; i++) {
		std::string hex_value;
		if (!is_valid_bigint_obj(L, i, hex_value)) {
			luaL_argcheck(L, false, i, "bigint value expected");
		}
		auto int_str = boost::algorithm::unhex(hex_value);
		boost::multiprecision::int256_t int_value(int_str);
		if (int_value < min_value) {
			imin = i;
			min_value = int_value;
		}
	}
	lua_pushvalue(L, imin);
	return 1;
}


static int safemath_max(lua_State *L) {
	int n = lua_gettop(L);  /* number of arguments */
	int imax = 1;  /* index of current max value */
	std::string first_hex_str;
	if (!is_valid_bigint_obj(L, 1, first_hex_str)) {
		luaL_argcheck(L, false, 1, "bigint value expected");
	}
	auto first_int_str = boost::algorithm::unhex(first_hex_str);
	boost::multiprecision::int256_t max_value(first_int_str);
	luaL_argcheck(L, n >= 1, 1, "value expected");
	for (int i = 2; i <= n; i++) {
		std::string hex_value;
		if (!is_valid_bigint_obj(L, i, hex_value)) {
			luaL_argcheck(L, false, i, "bigint value expected");
		}
		auto int_str = boost::algorithm::unhex(hex_value);
		boost::multiprecision::int256_t int_value(int_str);
		if (int_value > max_value) {
			imax = i;
			max_value = int_value;
		}
	}
	lua_pushvalue(L, imax);
	return 1;
}


static const luaL_Reg safemathlib[] = {
	{ "bigint", safemath_bigint },
	{ "add", safemath_add },
	{ "sub", safemath_sub },
	{ "mul", safemath_mul },
	{ "min", safemath_min },
	{ "max", safemath_max },
	{ "div", safemath_div },
	{ "rem", safemath_rem },
	{ "pow", safemath_pow },
	{ "toint", safemath_toint },
	{ "tohex", safemath_tohex },
	{ "tostring", safemath_tostring },
	{ nullptr, nullptr }
};


/*
** Open math library
*/
LUAMOD_API int luaopen_safemath(lua_State *L) {
	luaL_newlib(L, safemathlib);
	return 1;
}

