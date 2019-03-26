﻿#include <uvm/lprefix.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <sstream>
#include <utility>
#include <list>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <uvm/uvm_api.h>
#include <uvm/uvm_lib.h>
#include <uvm/uvm_lutil.h>
#include <uvm/lobject.h>
#include <uvm/lstate.h>
#include <uvm/exceptions.h>
#include <fc/crypto/sha1.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/hex.hpp>
#include <Keccak.hpp>
#include <simplechain/simplechain_uvm_api.h>
#include <simplechain/evaluate_state.h>
#include <simplechain/contract_evaluate.h>
#include <simplechain/contract_object.h>
#include <simplechain/blockchain.h>
#include <simplechain/address_helper.h>

namespace simplechain {
	using namespace uvm::lua::api;

			static int has_error = 0;

			/**
			* whether exception happen in L
			*/
			bool SimpleChainUvmChainApi::has_exception(lua_State *L)
			{
				return has_error ? true : false;
			}

			/**
			* clear exception marked
			*/
			void SimpleChainUvmChainApi::clear_exceptions(lua_State *L)
			{
				has_error = 0;
			}

			/**
			* when exception happened, use this api to tell uvm
			* @param L the lua stack
			* @param code error code, 0 is OK, other is different error
			* @param error_format error info string, will be released by lua
			* @param ... error arguments
			*/
			void SimpleChainUvmChainApi::throw_exception(lua_State *L, int code, const char *error_format, ...)
			{
				if (has_error)
					return;
				has_error = 1;
				char *msg = (char*)lua_malloc(L, LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH);
				if (msg) {
					memset(msg, 0x0, LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH);

					va_list vap;
					va_start(vap, error_format);
					vsnprintf(msg, LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH, error_format, vap);
					va_end(vap);
					if (strlen(msg) > LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH - 1)
					{
						msg[LUA_EXCEPTION_MULTILINE_STRNG_MAX_LENGTH - 1] = 0;
					}
				}
				else {
					msg = "vm out of memory";
				}
				lua_set_compile_error(L, msg);

				//如果上次的exception code为uvm_API_LVM_LIMIT_OVER_ERROR, 不能被其他异常覆盖
				//只有调用clear清理后，才能继续记录异常
				auto last_code = uvm::lua::lib::get_lua_state_value(L, "exception_code").int_value;
				if (last_code != code && last_code != 0)
				{
					return;
				}

				UvmStateValue val_code;
				val_code.int_value = code;

				UvmStateValue val_msg;
				val_msg.string_value = msg;
				printf("error: %s\n", msg);
				uvm::lua::lib::set_lua_state_value(L, "exception_code", val_code, UvmStateValueType::LUA_STATE_VALUE_INT);
				uvm::lua::lib::set_lua_state_value(L, "exception_msg", val_msg, UvmStateValueType::LUA_STATE_VALUE_STRING);
			}

			static contract_create_evaluator* get_register_contract_evaluator(lua_State *L) {
				return (contract_create_evaluator*)uvm::lua::lib::get_lua_state_value(L, "register_evaluate_state").pointer_value;
			}

			static native_contract_create_evaluator* get_native_register_contract_evaluator(lua_State *L) {
				return (native_contract_create_evaluator*)uvm::lua::lib::get_lua_state_value(L, "native_register_evaluate_state").pointer_value;
			}

			static contract_invoke_evaluator* get_invoke_contract_evaluator(lua_State *L) {
				return (contract_invoke_evaluator*)uvm::lua::lib::get_lua_state_value(L, "invoke_evaluate_state").pointer_value;
			}

			static evaluate_state* get_contract_evaluator(lua_State *L) {
				auto register_contract_evaluator = get_register_contract_evaluator(L);
				if (register_contract_evaluator) {
					return register_contract_evaluator;
				}
				auto native_register_contract_evaluator = get_native_register_contract_evaluator(L);
				if (native_register_contract_evaluator) {
					return native_register_contract_evaluator;
				}
				auto invoke_contract_evaluator = get_invoke_contract_evaluator(L);
				if (invoke_contract_evaluator) {
					return invoke_contract_evaluator;
				}
				return nullptr;
			}

			/**
			* check whether the contract apis limit over, in this lua_State
			* @param L the lua stack
			* @return TRUE(1 or not 0) if over limit(will break the vm), FALSE(0) if not over limit
			*/
			int SimpleChainUvmChainApi::check_contract_api_instructions_over_limit(lua_State *L)
			{
				try {
					auto evaluator = get_contract_evaluator(L);
					if (evaluator) {
						auto gas_limit = evaluator->gas_limit;
						if (gas_limit == 0)
							return 0;
						auto gas_count = uvm::lua::lib::get_lua_state_instructions_executed_count(L);
						return gas_count > gas_limit;
					}
					return 0;
				}FC_CAPTURE_AND_LOG((0))
			}

			static std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(evaluate_state* evaluator, const std::string& contract_id) {
				try {
					if (evaluator) {
						auto contract = evaluator->get_contract_by_address(contract_id);
						if (contract) {
							return std::make_shared<uvm::blockchain::Code>(contract->code);
						}
						else {
							return nullptr;
						}
					}
					return nullptr;
				}FC_CAPTURE_AND_LOG((contract_id))
			}

			static std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_name(evaluate_state* evaluator, const std::string& contract_name) {
				try {
					if (evaluator) {
						auto contract = evaluator->get_contract_by_name(contract_name);
						if (contract) {
							return std::make_shared<uvm::blockchain::Code>(contract->code);
						}
						else {
							return nullptr;
						}
					}
					return nullptr;
				}FC_CAPTURE_AND_LOG((contract_name))
			}

			static void put_contract_storage_changes_to_evaluator(evaluate_state* evaluator, const std::string& contract_id, const contract_storage_changes_type& changes) {
				try {
					if (evaluator) {
						evaluator->set_contract_storage_changes(contract_id, changes);
					}
				}FC_CAPTURE_AND_LOG((contract_id))
			}
			static std::shared_ptr<UvmContractInfo> get_contract_info_by_id(evaluate_state* evaluator, const std::string& contract_id) {
				try {
					if (evaluator) {
						auto contract = evaluator->get_contract_by_address(contract_id);
						if (contract) {
							auto contract_info = std::make_shared<UvmContractInfo>();
							for (const auto& api : contract->code.abi) {
								contract_info->contract_apis.push_back(api);
							}
							return contract_info;
						}
						else {
							return nullptr;
						}
					}
					return nullptr;
				}FC_CAPTURE_AND_LOG((contract_id))
			}

			static std::shared_ptr<contract_object> get_contract_object_by_name(evaluate_state* evaluator, const std::string& contract_name) {
				try {
					if (evaluator) {
						auto contract = evaluator->get_contract_by_name(contract_name);
						return contract;
					}
					return nullptr;
				}FC_CAPTURE_AND_LOG((contract_name))

			}

			int SimpleChainUvmChainApi::get_stored_contract_info(lua_State *L, const char *name, std::shared_ptr<UvmContractInfo> contract_info_ret)
			{
				auto evaluator = get_contract_evaluator(L);
				auto code = get_contract_code_by_name(evaluator, std::string(name));
				auto contract_info = get_contract_info_by_id(evaluator, std::string(name));
				if (!code)
					return 0;

				std::string addr_str(name);

				return get_stored_contract_info_by_address(L, addr_str.c_str(), contract_info_ret);
			}
			int SimpleChainUvmChainApi::get_stored_contract_info_by_address(lua_State *L, const char *contract_id, std::shared_ptr<UvmContractInfo> contract_info_ret)
			{
				auto evaluator = get_contract_evaluator(L);
				auto code = get_contract_code_by_id(evaluator, std::string(contract_id));
				auto contract_info = get_contract_info_by_id(evaluator, std::string(contract_id));
				if (!code || !contract_info)
					return 0;

				contract_info_ret->contract_apis.clear();

				std::copy(contract_info->contract_apis.begin(), contract_info->contract_apis.end(), std::back_inserter(contract_info_ret->contract_apis));
				std::copy(code->offline_abi.begin(), code->offline_abi.end(), std::back_inserter(contract_info_ret->contract_apis));
				return 1;
			}

			std::shared_ptr<UvmModuleByteStream> SimpleChainUvmChainApi::get_bytestream_from_code(lua_State *L, const uvm::blockchain::Code& code)
			{
				if (code.code.size() > LUA_MODULE_BYTE_STREAM_BUF_SIZE)
					return nullptr;
				auto p_luamodule = std::make_shared<UvmModuleByteStream>();
				p_luamodule->is_bytes = true;
				p_luamodule->buff.resize(code.code.size());
				memcpy(p_luamodule->buff.data(), code.code.data(), code.code.size());
				p_luamodule->contract_name = "";

				p_luamodule->contract_apis.clear();
				std::copy(code.abi.begin(), code.abi.end(), std::back_inserter(p_luamodule->contract_apis));

				p_luamodule->contract_emit_events.clear();
				std::copy(code.offline_abi.begin(), code.offline_abi.end(), std::back_inserter(p_luamodule->offline_apis));

				p_luamodule->contract_emit_events.clear();
				std::copy(code.events.begin(), code.events.end(), std::back_inserter(p_luamodule->contract_emit_events));

				p_luamodule->contract_storage_properties.clear();
				for (const auto &p : code.storage_properties)
				{
					p_luamodule->contract_storage_properties[p.first] = p.second;
				}
				return p_luamodule;
			}

			void SimpleChainUvmChainApi::get_contract_address_by_name(lua_State *L, const char *name, char *address, size_t *address_size)
			{
				auto evaluator = get_contract_evaluator(L);
				std::string contract_name = uvm::lua::lib::unwrap_any_contract_name(name);
				auto is_address = is_valid_contract_address(L, name);
				auto code = is_address ? get_contract_code_by_id(evaluator, contract_name) : get_contract_code_by_name(evaluator, contract_name);
				auto contract_info_by_id = get_contract_info_by_id(evaluator, contract_name);
				auto contract_info_by_name = get_contract_object_by_name(evaluator, contract_name);
				auto contract_addr = is_address ? (contract_info_by_id != nullptr ? contract_name : "") : (contract_info_by_name ? contract_info_by_name->contract_address : "");
				if (code && !contract_addr.empty())
				{
					std::string address_str = contract_addr;
					*address_size = address_str.length();
					strncpy(address, address_str.c_str(), CONTRACT_ID_MAX_LENGTH - 1);
					address[CONTRACT_ID_MAX_LENGTH - 1] = '\0';
					*address_size = strlen(address);
				}
			}
            
            bool SimpleChainUvmChainApi::check_contract_exist_by_address(lua_State *L, const char *address)
            {
				auto evaluator = get_contract_evaluator(L);
				auto code = get_contract_code_by_id(evaluator, std::string(address));
				return code ? true : false;
            }

			bool SimpleChainUvmChainApi::check_contract_exist(lua_State *L, const char *name)
			{
				auto evaluator = get_contract_evaluator(L);
				auto code = get_contract_code_by_name(evaluator, std::string(name));
				return code ? true : false;
			}

			/**
			* load contract lua byte stream from uvm api
			*/
			std::shared_ptr<UvmModuleByteStream> SimpleChainUvmChainApi::open_contract(lua_State *L, const char *name)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);

				auto evaluator = get_contract_evaluator(L);
				std::string contract_name = uvm::lua::lib::unwrap_any_contract_name(name);

				auto code = get_contract_code_by_name(evaluator, contract_name);
				if (code && (code->code.size() <= LUA_MODULE_BYTE_STREAM_BUF_SIZE))
				{
					return get_bytestream_from_code(L, *code);
				}

				return nullptr;
			}
            
			std::shared_ptr<UvmModuleByteStream> SimpleChainUvmChainApi::open_contract_by_address(lua_State *L, const char *address)
            {
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				auto evaluator = get_contract_evaluator(L);
				auto code = get_contract_code_by_id(evaluator, std::string(address));
				if (code && (code->code.size() <= LUA_MODULE_BYTE_STREAM_BUF_SIZE))
				{
					return get_bytestream_from_code(L, *code);
				}

				return nullptr;
            }

            // storage,mapkey contract_id + "$" + storage_name
            // TODO: lua_closepost_callback，
            static std::map<lua_State *, std::shared_ptr<std::map<std::string, UvmStorageValue>>> _demo_chain_storage_buffer;

			UvmStorageValue SimpleChainUvmChainApi::get_storage_value_from_uvm(lua_State *L, const char *contract_name, const std::string& name,
				const std::string& fast_map_key, bool is_fast_map)
			{
				UvmStorageValue null_storage;
				null_storage.type = uvm::blockchain::StorageValueTypes::storage_value_null;

				auto evaluator = get_contract_evaluator(L);
				std::string contract_id = uvm::lua::lib::unwrap_any_contract_name(contract_name);
				auto code = get_contract_code_by_id(evaluator, contract_id);
				if (!code)
				{
					return null_storage;
				}
				try
				{
					return get_storage_value_from_uvm_by_address(L, contract_id.c_str(), name , fast_map_key, is_fast_map );
				}
				catch (...) {
					return null_storage;
				}
			}

			static bool compare_key(const std::string& first, const std::string& second)
			{
				unsigned int i = 0;
				while ((i<first.length()) && (i<second.length()))
				{
					if (first[i] < second[i])
						return true;
					else if (first[i] > second[i])
						return false;
					else
						++i;
				}
				return (first.length() < second.length());
			}

			// parse arg to json_array when it's json object. otherwhile return itself. And recursively process child elements
			static jsondiff::JsonValue nested_json_object_to_array(const jsondiff::JsonValue& json_value)
			{
				if (json_value.is_object())
				{
					const auto& obj = json_value.as<jsondiff::JsonObject>();
					jsondiff::JsonArray json_array;
					std::list<std::string> keys;
					for (auto it = obj.begin(); it != obj.end(); it++)
					{
						keys.push_back(it->key());
					}
					keys.sort(&compare_key);
					for (const auto& key : keys)
					{
						jsondiff::JsonArray item_json;
						item_json.push_back(key);
						item_json.push_back(nested_json_object_to_array(obj[key]));
						json_array.push_back(item_json);
					}
					return json_array;
				}
				if (json_value.is_array())
				{
					const auto& arr = json_value.as<jsondiff::JsonArray>();
					jsondiff::JsonArray result;
					for (const auto& item : arr)
					{
						result.push_back(nested_json_object_to_array(item));
					}
					return result;
				}
				return json_value;
			}

			static cbor::CborObjectP nested_cbor_object_to_array(const cbor::CborObject* cbor_value)
			{
				if (cbor_value->is_map())
				{
					const auto& map = cbor_value->as_map();
					cbor::CborArrayValue cbor_array;
					std::list<std::string> keys;
					for (auto it = map.begin(); it != map.end(); it++)
					{
						keys.push_back(it->first);
					}
					keys.sort(&compare_key);
					for (const auto& key : keys)
					{
						cbor::CborArrayValue item_json;
						item_json.push_back(cbor::CborObject::from_string(key));
						item_json.push_back(nested_cbor_object_to_array(map.at(key).get()));
						cbor_array.push_back(cbor::CborObject::create_array(item_json));
					}
					return cbor::CborObject::create_array(cbor_array);
				}
				if (cbor_value->is_array())
				{
					const auto& arr = cbor_value->as_array();
					cbor::CborArrayValue result;
					for (const auto& item : arr)
					{
						result.push_back(nested_cbor_object_to_array(item.get()));
					}
					return cbor::CborObject::create_array(result);
				}
				return std::make_shared<cbor::CborObject>(*cbor_value);
			}


			UvmStorageValue SimpleChainUvmChainApi::get_storage_value_from_uvm_by_address(lua_State *L, const char *contract_address, const std::string& name
				, const std::string& fast_map_key, bool is_fast_map)
			{
				UvmStorageValue null_storage;
				null_storage.type = uvm::blockchain::StorageValueTypes::storage_value_null;

				auto evaluator = get_contract_evaluator(L);
				std::string contract_id(contract_address);
				auto code = get_contract_code_by_id(evaluator, contract_id);
				if (!code)
				{
					return null_storage;
				}
				try
				{
					std::string key = name;
					
					if (is_fast_map) {
						key = name + "." + fast_map_key;
					}
					
					auto storage_data = evaluator->get_storage(contract_id, key);
					return StorageDataType::create_lua_storage_from_storage_data(L, storage_data);
				}
				catch (...) {
					return null_storage;
				}
			}

			static std::vector<char> json_to_chars(const jsondiff::JsonValue& json_value)
			{
				const auto &json_str = jsondiff::json_dumps(json_value);
				std::vector<char> data(json_str.size() + 1);
				memcpy(data.data(), json_str.c_str(), json_str.size());
				data[json_str.size()] = '\0';
				return data;
			}
			
			bool SimpleChainUvmChainApi::commit_storage_changes_to_uvm(lua_State *L, AllContractsChangesMap &changes)
			{
				auto evaluator = get_contract_evaluator(L);
				//auto use_cbor_diff_flag = use_cbor_diff(L);
				auto use_cbor_diff_flag = true;
				cbor_diff::CborDiff differ;
				jsondiff::JsonDiff json_differ;
				int64_t storage_gas = 0;

				auto gas_limit = uvm::lua::lib::get_lua_state_instructions_limit(L);
				const char* out_of_gas_error = "contract storage changes out of gas";

				for (auto all_con_chg_iter = changes.begin(); all_con_chg_iter != changes.end(); ++all_con_chg_iter)
				{
					// commit change to evaluator
					contract_storage_changes_type contract_storage_change;
					std::string contract_id = all_con_chg_iter->first;
					ContractChangesMap contract_change = *(all_con_chg_iter->second);
					cbor::CborMapValue nested_changes;
					jsondiff::JsonObject json_nested_changes;

					for (auto con_chg_iter = contract_change.begin(); con_chg_iter != contract_change.end(); ++con_chg_iter)
					{
						std::string contract_name = con_chg_iter->first;

						StorageDataChangeType storage_change;
						// storage_op存储的从before, after改成diff
						auto storage_after = StorageDataType::get_storage_data_from_lua_storage(con_chg_iter->second.after);
						if (use_cbor_diff_flag) {
							auto cbor_storage_before = uvm_storage_value_to_cbor(con_chg_iter->second.before);
							auto cbor_storage_after = uvm_storage_value_to_cbor(con_chg_iter->second.after);
							con_chg_iter->second.cbor_diff = *(differ.diff(cbor_storage_before, cbor_storage_after));
							auto cbor_diff_value = std::make_shared<cbor::CborObject>(con_chg_iter->second.cbor_diff.value());
							const auto& cbor_diff_chars = cbor_diff::cbor_encode(cbor_diff_value);
							storage_change.storage_diff.storage_data = cbor_diff_chars;
							storage_change.after = storage_after;
							contract_storage_change[contract_name] = storage_change;
							nested_changes[contract_name] = cbor_diff_value;
						}
						else {
							
							const auto& json_storage_before = simplechain::uvm_storage_value_to_json(con_chg_iter->second.before);
							const auto& json_storage_after = simplechain::uvm_storage_value_to_json(con_chg_iter->second.after);
							con_chg_iter->second.diff = *(json_differ.diff(json_storage_before, json_storage_after));
							storage_change.storage_diff.storage_data = json_to_chars(con_chg_iter->second.diff.value());
							storage_change.after = storage_after;
							contract_storage_change[contract_name] = storage_change;
							json_nested_changes[contract_name] = con_chg_iter->second.diff.value();
							
						}

					}
					// count gas by changes size
					size_t changes_size = 0;
					if (use_cbor_diff_flag) {
						auto nested_changes_cbor = cbor::CborObject::create_map(nested_changes);
						const auto& changes_parsed_to_array = nested_cbor_object_to_array(nested_changes_cbor.get());
						changes_size = cbor_diff::cbor_encode(changes_parsed_to_array).size();
					}
					/*else {
						const auto& changes_parsed_to_array = nested_json_object_to_array(json_nested_changes);
						changes_size = jsondiff::json_dumps(changes_parsed_to_array).size();
					}*/
					// printf("changes size: %d bytes\n", changes_size);
					storage_gas += changes_size * 10; // 1 byte storage cost 10 gas
					if (storage_gas < 0 && gas_limit > 0) {
						throw_exception(L, UVM_API_LVM_LIMIT_OVER_ERROR, out_of_gas_error);
						return false;
					}
					put_contract_storage_changes_to_evaluator(evaluator, contract_id, contract_storage_change);
				}
				if (gas_limit > 0) {
					if (storage_gas > gas_limit || storage_gas + uvm::lua::lib::get_lua_state_instructions_executed_count(L) > gas_limit) {
						throw_exception(L, UVM_API_LVM_LIMIT_OVER_ERROR, out_of_gas_error);
						return false;
					}
				}
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, storage_gas);
				return true;
			}
			intptr_t SimpleChainUvmChainApi::register_object_in_pool(lua_State *L, intptr_t object_addr, UvmOutsideObjectTypes type)
			{
				auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
				// Map<type, Map<object_key, object_addr>>
				std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
				if (node.type == UvmStateValueType::LUA_STATE_VALUE_nullptr)
				{
					node.type = UvmStateValueType::LUA_STATE_VALUE_POINTER;
					object_pools = new std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>>();
					node.value.pointer_value = (void*)object_pools;
					uvm::lua::lib::set_lua_state_value(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY, node.value, node.type);
				}
				else
				{
					object_pools = (std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
				}
				if (object_pools->find(type) == object_pools->end())
				{
					object_pools->emplace(std::make_pair(type, std::make_shared<std::map<intptr_t, intptr_t>>()));
				}
				auto pool = (*object_pools)[type];
				auto object_key = object_addr;
				(*pool)[object_key] = object_addr;
				return object_key;
			}

			intptr_t SimpleChainUvmChainApi::is_object_in_pool(lua_State *L, intptr_t object_key, UvmOutsideObjectTypes type)
			{
				auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
				// Map<type, Map<object_key, object_addr>>
				std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
				if (node.type == UvmStateValueType::LUA_STATE_VALUE_nullptr)
				{
					return 0;
				}
				else
				{
					object_pools = (std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
				}
				if (object_pools->find(type) == object_pools->end())
				{
					object_pools->emplace(std::make_pair(type, std::make_shared<std::map<intptr_t, intptr_t>>()));
				}
				auto pool = (*object_pools)[type];
				return (*pool)[object_key];
			}

			void SimpleChainUvmChainApi::release_objects_in_pool(lua_State *L)
			{
				auto node = uvm::lua::lib::get_lua_state_value_node(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY);
				// Map<type, Map<object_key, object_addr>>
				std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *object_pools = nullptr;
				if (node.type == UvmStateValueType::LUA_STATE_VALUE_nullptr)
				{
					return;
				}
				object_pools = (std::map<UvmOutsideObjectTypes, std::shared_ptr<std::map<intptr_t, intptr_t>>> *) node.value.pointer_value;
				// TODO: 对于object_pools中不同类型的对象，分别释放
				for (const auto &p : *object_pools)
				{
					auto type = p.first;
					auto pool = p.second;
					for (const auto &object_item : *pool)
					{
						auto object_key = object_item.first;
						auto object_addr = object_item.second;
						if (object_addr == 0)
							continue;
						switch (type)
						{
						case UvmOutsideObjectTypes::OUTSIDE_STREAM_STORAGE_TYPE:
						{
							auto stream = (uvm::lua::lib::UvmByteStream*) object_addr;
							delete stream;
						} break;
						default: {
							continue;
						}
						}
					}
				}
				delete object_pools;
				UvmStateValue null_state_value;
				null_state_value.int_value = 0;
				uvm::lua::lib::set_lua_state_value(L, GLUA_OUTSIDE_OBJECT_POOLS_KEY, null_state_value, UvmStateValueType::LUA_STATE_VALUE_nullptr);
			}

			bool SimpleChainUvmChainApi::register_storage(lua_State *L, const char *contract_name, const char *name)
			{
				return true;
			}

			lua_Integer SimpleChainUvmChainApi::transfer_from_contract_to_address(lua_State *L, const char *contract_address, const char *to_address,
				const char *asset_type, int64_t amount)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				contract_address_type f_addr;
				address t_addr;
				try {
					f_addr = contract_address_type(contract_address);
				}
				catch (...)
				{
					return -3;
				}
				try {
					t_addr = address(to_address);
				}
				catch (...)
				{
					return -4;
				}
				if (amount <= 0)
					return -6;
				auto evaluator = get_contract_evaluator(L);
				try {
					auto asset_item = evaluator->get_chain()->get_asset_by_symbol(asset_type);
					if (!asset_item) {
						return -5;
					}
					auto contract_balance = evaluator->get_account_asset_balance(contract_address, asset_item->asset_id);
					if (contract_balance < static_cast<share_type>(amount)) {
						return -1;
					}
					evaluator->update_account_asset_balance(contract_address, asset_item->asset_id, -amount);
					evaluator->update_account_asset_balance(to_address, asset_item->asset_id, amount);
				}
				catch (...)
				{
					return -1;
				}
				return 0;
			}

			lua_Integer SimpleChainUvmChainApi::transfer_from_contract_to_public_account(lua_State *L, const char *contract_address, const char *to_account_name,
				const char *asset_type, int64_t amount)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				// TODO: not implemented yet
				return -1;
			}

			int64_t SimpleChainUvmChainApi::get_contract_balance_amount(lua_State *L, const char *contract_address, const char* asset_symbol)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				contract_address_type c_addr;
				try {
					c_addr = contract_address_type(contract_address);
				}
				catch (...)
				{
					return -2;
				}
				try {

					auto evaluator = get_contract_evaluator(L);
					try {
						auto asset_item = evaluator->get_chain()->get_asset_by_symbol(asset_symbol);
						if (!asset_item) {
							return -3;
						}
						auto balance = evaluator->get_account_asset_balance(contract_address, asset_item->asset_id);
						return balance;
					}
					catch (...)
					{
						return -1;
					}
					return 0;
				}
				catch (const fc::exception& e)
				{
					switch (e.code())
					{
					case 30028://invalid_address
						return -2;
						//case 31003://unknown_balance_entry
						//    return -3;
					case 31303:
						return -1;
					default:
						L->force_stopping = true;
						L->exit_code = LUA_API_INTERNAL_ERROR;
						return -4;
						break;
					}
				}
				catch (...) {
					L->force_stopping = true;
					L->exit_code = LUA_API_INTERNAL_ERROR;
					return -4;
				}
			}

			int64_t SimpleChainUvmChainApi::get_transaction_fee(lua_State *L)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				try {
					return 0;
				}
				catch (...)
				{
					L->force_stopping = true;
					L->exit_code = LUA_API_INTERNAL_ERROR;
					return -2;
				}
			}

			uint32_t SimpleChainUvmChainApi::get_chain_now(lua_State *L)
			{				
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				try {
					auto evaluator = get_contract_evaluator(L);
					return evaluator->get_chain()->latest_block().block_time.sec_since_epoch();
				}
				catch (...)
				{
					L->force_stopping = true;
					L->exit_code = LUA_API_INTERNAL_ERROR;
					return 0;
				}
			}

			uint32_t SimpleChainUvmChainApi::get_chain_random(lua_State *L)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				try {
					auto evaluator = get_contract_evaluator(L);
					const auto& chain = evaluator->get_chain();
					const auto& block = chain->latest_block();
					auto hash = block.digest();
					return hash._hash[3] % ((1 << 31) - 1);
				}
				catch (...)
				{
					L->force_stopping = true;
					L->exit_code = LUA_API_INTERNAL_ERROR;
					return 0;
				}
			}

			std::string SimpleChainUvmChainApi::get_transaction_id(lua_State *L)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				try {
					auto evaluator = get_contract_evaluator(L);
					return evaluator->get_current_tx()->tx_hash();
				}
				catch (...)
				{
					L->force_stopping = true;
					L->exit_code = LUA_API_INTERNAL_ERROR;
					return "";
				}
			}

			uint32_t SimpleChainUvmChainApi::get_header_block_num(lua_State *L)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				try {
					auto evaluator = get_contract_evaluator(L);
					return (uint32_t) evaluator->get_chain()->latest_block().block_number;
				}
				catch (...)
				{
					L->force_stopping = true;
					L->exit_code = LUA_API_INTERNAL_ERROR;
					return 0;
				}
			}

			uint32_t SimpleChainUvmChainApi::wait_for_future_random(lua_State *L, int next)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				try {
					auto evaluator = get_contract_evaluator(L);
					const auto& chain = evaluator->get_chain();
					auto target = chain->latest_block().block_number + next;
					if (target < next)
						return 0;
					return static_cast<uint32_t>(target);
				}
				catch (...)
				{
					L->force_stopping = true;
					L->exit_code = LUA_API_INTERNAL_ERROR;
					return 0;
				}
			}

			int32_t SimpleChainUvmChainApi::get_waited(lua_State *L, uint32_t num)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				try {
					if (num <= 1)
						return -2;
					auto evaluator = get_contract_evaluator(L);
					const auto& chain = evaluator->get_chain();
					if (chain->latest_block().block_number < num)
						return -1;
					const auto& block = chain->get_block_by_number(num);
					auto hash = block->digest();
					return hash._hash[3] % ((1 << 31) - 1);
				}
				catch (...)
				{
					L->force_stopping = true;
					L->exit_code = LUA_API_INTERNAL_ERROR;
					return -1;
				}
			}

			void SimpleChainUvmChainApi::emit(lua_State *L, const char* contract_id, const char* event_name, const char* event_param)
			{
				uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT - 1);
				try {
					auto evaluator = get_contract_evaluator(L);
					contract_address_type contract_addr(contract_id);
					evaluator->emit_event(contract_addr, std::string(event_name), std::string(event_param));
				}
				catch (...)
				{
					L->force_stopping = true;
					L->exit_code = LUA_API_INTERNAL_ERROR;
					return;
				}
			}

			bool SimpleChainUvmChainApi::is_valid_address(lua_State *L, const char *address_str)
			{
				std::string addr(address_str);
				return helper::is_valid_address(addr);
			}

			bool SimpleChainUvmChainApi::is_valid_contract_address(lua_State *L, const char *address_str)
			{
				std::string addr(address_str);
				return helper::is_valid_contract_address(addr);
			}

			const char * SimpleChainUvmChainApi::get_system_asset_symbol(lua_State *L)
			{
				auto evaluator = get_contract_evaluator(L);
				return SIMPLECHAIN_CORE_ASSET_SYMBOL;
			}

			uint64_t SimpleChainUvmChainApi::get_system_asset_precision(lua_State *L)
			{
				return static_cast<uint64_t>(std::pow(10, SIMPLECHAIN_CORE_ASSET_PRECISION));
			}

			static std::vector<char> hex_to_chars(const std::string& hex_string) {
				std::vector<char> chars(hex_string.size() / 2);
				auto bytes_count = fc::from_hex(hex_string, chars.data(), chars.size());
				if (bytes_count != chars.size()) {
					throw uvm::core::UvmException("parse hex to bytes error");
				}
				return chars;
			}

			std::vector<unsigned char> SimpleChainUvmChainApi::hex_to_bytes(const std::string& hex_string) {
				const auto& chars = hex_to_chars(hex_string);
				std::vector<unsigned char> bytes(chars.size());
				memcpy(bytes.data(), chars.data(), chars.size());
				return bytes;
			}
			std::string SimpleChainUvmChainApi::bytes_to_hex(std::vector<unsigned char> bytes) {
				std::vector<char> chars(bytes.size());
				memcpy(chars.data(), bytes.data(), bytes.size());
				return fc::to_hex(chars);
			}
			std::string SimpleChainUvmChainApi::sha256_hex(const std::string& hex_string) {
				const auto& chars = hex_to_chars(hex_string);
				auto hash_result = fc::sha256::hash(chars.data(), static_cast<uint32_t>(chars.size()));
				return hash_result.str();
			}
			std::string SimpleChainUvmChainApi::sha1_hex(const std::string& hex_string) {
				const auto& chars = hex_to_chars(hex_string);
				auto hash_result = fc::sha1::hash(chars.data(), static_cast<uint32_t>(chars.size()));
				return hash_result.str();
			}
			std::string SimpleChainUvmChainApi::sha3_hex(const std::string& hex_string) {
				Keccak keccak(Keccak::Keccak256);
				const auto& chars = hex_to_chars(hex_string);
				auto hash_result = keccak(chars.data(), chars.size());
				return hash_result;
			}
			std::string SimpleChainUvmChainApi::ripemd160_hex(const std::string& hex_string) {
				const auto& chars = hex_to_chars(hex_string);
				auto hash_result = fc::ripemd160::hash(chars.data(), static_cast<uint32_t>(chars.size()));
				return hash_result.str();
			}

			std::string SimpleChainUvmChainApi::get_address_role(lua_State* L, const std::string& addr) {
				return "address";
			}

			int64_t SimpleChainUvmChainApi::get_fork_height(lua_State* L, const std::string& fork_key) {
				return -1;
			}

			bool SimpleChainUvmChainApi::use_cbor_diff(lua_State* L) const {
				if (1 == L->cbor_diff_state) {
					return true;
				}
				else if (2 == L->cbor_diff_state) {
					return false;
				}
				auto result = true;
				L->cbor_diff_state = result ? 1 : 2; // cache it
				return result;
			}

			std::string SimpleChainUvmChainApi::pubkey_to_address_string(const fc::ecc::public_key& pub) const {
				auto dat = pub.serialize();
				auto addr = fc::ripemd160::hash(fc::sha512::hash(dat.data, sizeof(dat)));
				std::string prefix = "SL";
				unsigned char version = 0X35;
				fc::array<char, 25> bin_addr;
				memcpy((char*)&bin_addr, (char*)&version, sizeof(version));
				memcpy((char*)&bin_addr + 1, (char*)&addr, sizeof(addr));
				auto checksum = fc::ripemd160::hash((char*)&bin_addr, bin_addr.size() - 4);
				memcpy(((char*)&bin_addr) + 21, (char*)&checksum._hash[0], 4);
				return prefix + fc::to_base58(bin_addr.data, sizeof(bin_addr));
			}

}
