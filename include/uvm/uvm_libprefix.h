#pragma once


#define BOOL_VAL(val) ((val)>0?true:false)

#define ADDRESS_CONTRACT_PREFIX "@pointer_"

#define STREAM_CONTRACT_PREFIX "@stream_"

/**
 * wrapper contract name, so it wan't conflict with lua inner module names
 */
#define CONTRACT_NAME_WRAPPER_PREFIX "@g_"

#define CURRENT_CONTRACT_NAME	"@self"

#define UVM_CONTRACT_INITING "uvm_contract_ininting"

#define STARTING_CONTRACT_ADDRESS "starting_contract_address"

#define LUA_STATE_DEBUGGER_INFO	"lua_state_debugger_info"

#define GLUA_TYPE_NAMESPACE_PREFIX "$type$"
#define GLUA_TYPE_NAMESPACE_PREFIX_WRAP(name) "$type$" #name ""
