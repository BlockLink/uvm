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



#define LOG_INFO(...)  fprintf(stderr, "[INFO] " ##__VA_ARGS__)

#define ERROR_INFO(...) fprintf(stderr, "[ERROR] " ##__VA_ARGS__)

#define LUA_MODULE_BYTE_STREAM_BUF_SIZE 1024*1024

#define LUA_VM_EXCEPTION_STRNG_MAX_LENGTH 256

#define LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH 4096

#define CONTRACT_ID_MAX_LENGTH 50

#define CONTRACT_NAME_MAX_LENGTH 50

#define CONTRACT_MAX_OFFLINE_API_COUNT	1024

#define LUA_FUNCTION_MAX_LOCALVARS_COUNT 128

#define LUA_MAX_LOCALVARNAME_LENGTH	128

 // emiteventTypeName
#define EMIT_EVENT_NAME_MAX_COUNT 50

//  
#define CONTRACT_LEVEL_TEMP 1
#define CONTRACT_LEVEL_FOREVER 2

//  
#define CONTRACT_STATE_VALID 1
#define CONTRACT_STATE_DELETED 2

// blockchain
#define USE_TYPE_CHECK true 

// storagerecord
#define CONTRACT_STORAGE_PROPERTIES_MAX_COUNT 256

// uvm API
#define CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT 50

#define CHECK_CONTRACT_CODE_EVERY_TIME 0




// storage structs
#define LUA_STORAGE_CHANGELIST_KEY "__lua_storage_changelist__"
#define LUA_STORAGE_READ_TABLES_KEY "__lua_storage_read_tables__"

#define GLUA_OUTSIDE_OBJECT_POOLS_KEY "__uvm_outside_object_pools__"