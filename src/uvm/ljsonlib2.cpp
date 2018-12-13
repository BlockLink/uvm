#define ljsonlib_cpp

#include <uvm/lprefix.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <cassert>

#include <uvm/lua.h>

#include <uvm/lauxlib.h>
#include <uvm/lualib.h>
#include <uvm/uvm_tokenparser.h>
#include <uvm/uvm_api.h>
#include <uvm/uvm_lib.h>
#include <uvm/uvm_lutil.h>

#include <uvm/json_reader.h>

using uvm::lua::api::global_uvm_chain_api;

using namespace uvm::parser;

static UvmStorageValue nil_storage_value()
{
	UvmStorageValue value;
	value.type = uvm::blockchain::StorageValueTypes::storage_value_null;
	value.value.int_value = 0;
	return value;
}


static int json_to_lua(lua_State *L)
{
	if (lua_gettop(L) < 1)
		return 0;
	if (!lua_isstring(L, 1))
		return 0;
	auto json_str = luaL_checkstring(L, 1);
	//uvm::lua::lib::UvmStateScope scope;
	auto json_parser = std::make_shared<Json_Reader>();
	UvmStorageValue root;
	if (json_parser->parse(L, std::string(json_str), &root)) {
		lua_push_storage_value(L, root);
		return 1;
	}
	else {
		global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "parse json error(%s)", json_parser->error.message_.c_str());
		return 0;
	}

}

static int lua_to_json(lua_State *L)
{
	if (lua_gettop(L) < 1)
		return 0;
	auto value = luaL_tojsonstring(L, 1, nullptr);
	lua_pushstring(L, value);
	printf("jsonlib2:%s",value);
	return 1;
}

static const luaL_Reg dblib[] = {
	{ "loads", json_to_lua },
{ "dumps", lua_to_json },
{ nullptr, nullptr }
};


LUAMOD_API int luaopen_json2(lua_State *L) {
	luaL_newlib(L, dblib);
	return 1;
}

