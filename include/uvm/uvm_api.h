/**
 * lua module injector header in uvm
 */

#ifndef uvm_api_h
#define uvm_api_h

#include <uvm/lprefix.h>

#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <memory>
#include <iostream>

#include <uvm/lua.h>
#include <jsondiff/jsondiff.h>
#include <jsondiff/exceptions.h>
#include <cborcpp/cbor.h>
#include <cbor_diff/cbor_diff.h>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>

#include <uvm/uvm_storage_value.h>
#include <uvm/uvm_module.h>


class UvmContractInfo
{
public:
    std::vector<std::string> contract_apis;
	std::map<std::string, std::vector<UvmTypeInfoEnum>> contract_api_arg_types;
};

// uvm api error codes
#define UVM_API_NO_ERROR 0
#define UVM_API_SIMPLE_ERROR 1
#define UVM_API_MEMORY_ERROR 2
#define UVM_API_LVM_ERROR 3
#define UVM_API_PARSER_ERROR 4
#define UVM_API_COMPILE_ERROR 5
#define UVM_API_LVM_LIMIT_OVER_ERROR 6
#define UVM_API_THROW_ERROR 7   // uvm


namespace uvm {
    namespace blockchain{
        struct Code;
    }

    namespace lua {
        namespace api {

          class IUvmChainApi
          {
          public:
            /**
            * check whether the contract apis limit over, in this lua_State
            * @param L the lua stack
            * @return TRUE(1 or not 0) if over limit(will break the vm), FALSE(0) if not over limit
            */
            virtual int check_contract_api_instructions_over_limit(lua_State *L) = 0;

            /**
             * whether exception happen in L
             */
            virtual bool has_exception(lua_State *L) = 0;

            /**
            * clear exception marked
            */
            virtual void clear_exceptions(lua_State *L) = 0;


            /**
            * when exception happened, use this api to tell uvm
            * @param L the lua stack
            * @param code error code, 0 is OK, other is different error
            * @param error_format error info string, will be released by lua
            * @param ... error arguments
            */
            virtual void throw_exception(lua_State *L, int code, const char *error_format, ...) = 0;

            /**
            * get contract info stored before from uvm api
            * @param name contract name
            * @param contract_info_ret this api save the contract's api name array here if found, this var will be free by this api
            * @return TRUE(1 or not 0) if success, FALSE(0) if failed
            */
            virtual int get_stored_contract_info(lua_State *L, const char *name, std::shared_ptr<UvmContractInfo> contract_info_ret) = 0;

            virtual int get_stored_contract_info_by_address(lua_State *L, const char *address, std::shared_ptr<UvmContractInfo> contract_info_ret) = 0;

            virtual std::shared_ptr<UvmModuleByteStream> get_bytestream_from_code(lua_State *L, const uvm::blockchain::Code& code) = 0;
            /**
            * load contract lua byte stream from uvm api
            */
            virtual std::shared_ptr<UvmModuleByteStream> open_contract(lua_State *L, const char *name) = 0;

            virtual std::shared_ptr<UvmModuleByteStream> open_contract_by_address(lua_State *L, const char *address) = 0;

            /**
             * get contract address/id from uvm by contract name
             */
            virtual void get_contract_address_by_name(lua_State *L, const char *name, char *address, size_t *address_size) = 0;

            /*
             * check whether the contract exist
             */
            virtual bool check_contract_exist(lua_State *L, const char *name) = 0;

            /**
             * check contract exist by ID string address
             */
            virtual bool check_contract_exist_by_address(lua_State *L, const char *address) = 0;

            /**
             * register new storage name of contract to uvm
             */
            virtual bool register_storage(lua_State *L, const char *contract_name, const char *name) = 0;

            virtual UvmStorageValue get_storage_value_from_uvm(lua_State *L, const char *contract_name,
				const std::string& name, const std::string& fast_map_key, bool is_fast_map) = 0;

            virtual UvmStorageValue get_storage_value_from_uvm_by_address(lua_State *L, const char *contract_address,
				const std::string& name, const std::string& fast_map_key, bool is_fast_map) = 0;

            /**
             * after lua merge storage changes in lua_State, use the function to store the merged changes of storage to uvm
             */
            virtual bool commit_storage_changes_to_uvm(lua_State *L, AllContractsChangesMap &changes) = 0;

            /**
             * lua_State（），
             * lua_State
             * uvm/，，lightuserdatauvm
             */
            virtual intptr_t register_object_in_pool(lua_State *L, intptr_t object_addr, UvmOutsideObjectTypes type) = 0;

            /**
             * (register_object_in_pool，)lua_State（)
             * ，，0
             */
            virtual intptr_t is_object_in_pool(lua_State *L, intptr_t object_key, UvmOutsideObjectTypes type) = 0;

            /**
             * lua_State
             */
            virtual void release_objects_in_pool(lua_State *L) = 0;

            /************************************************************************/
            /* transfer asset from contract by account address                      */
            /************************************************************************/
            virtual lua_Integer transfer_from_contract_to_address(lua_State *L, const char *contract_address, const char *to_address,
              const char *asset_type, int64_t amount) = 0;

			/**
			 * lockbalance and payback
			 */
			virtual bool lock_contract_balance_to_miner(lua_State *L, const char* cid, const char* asset_sym, const char* amount, const char* mid) {
				throw_exception(L, UVM_API_SIMPLE_ERROR, "no implemented method");
				return false;
			}
			virtual bool obtain_pay_back_balance(lua_State *L, const char* contract_addr, const char* mid, const char* sym_to_obtain, const char* amount) {
				throw_exception(L, UVM_API_SIMPLE_ERROR, "no implemented method");
				return false; 
			}
			virtual bool foreclose_balance_from_miners(lua_State *L, const char* foreclose_account, const char* mid, const char* sym_to_foreclose, const char* amount) {
				throw_exception(L, UVM_API_SIMPLE_ERROR, "no implemented method");
				return false;
			}
			virtual std::string get_contract_lock_balance_info(lua_State *L, const char* mid) {
				throw_exception(L, UVM_API_SIMPLE_ERROR, "no implemented method");
				return "";
			}
			virtual std::string get_contract_lock_balance_info(lua_State *L, const char* cid, const char* aid)const {
				return "";
			}
			virtual std::string get_pay_back_balance(lua_State *L, const char* contract_addr, const char* symbol_type) {
				throw_exception(L, UVM_API_SIMPLE_ERROR, "no implemented method");
				return "";
			}

            /************************************************************************/
            /* transfer asset from contract by account name on chain                */
            /************************************************************************/
            virtual lua_Integer transfer_from_contract_to_public_account(lua_State *L, const char *contract_address, const char *to_account_name,
              const char *asset_type, int64_t amount) = 0;

            virtual int64_t get_contract_balance_amount(lua_State *L, const char *contract_address, const char* asset_symbol) = 0;
            virtual int64_t get_transaction_fee(lua_State *L) = 0;
            virtual uint32_t get_chain_now(lua_State *L) = 0;
            virtual uint32_t get_chain_random(lua_State *L) = 0;
			virtual uint32_t get_chain_safe_random(lua_State *L, bool diff_in_diff_txs) = 0;
            virtual std::string get_transaction_id(lua_State *L) = 0;
			virtual std::string get_transaction_id_without_gas(lua_State *L) const = 0;
            virtual uint32_t get_header_block_num(lua_State *L) = 0;
			virtual uint32_t get_header_block_num_without_gas(lua_State *L) const = 0;
            virtual uint32_t wait_for_future_random(lua_State *L, int next) = 0;

            virtual int32_t get_waited(lua_State *L, uint32_t num) = 0;

            virtual void emit(lua_State *L, const char* contract_id, const char* event_name, const char* event_param) = 0;

			virtual bool is_valid_address(lua_State *L, const char *address_str) = 0;
			virtual bool is_valid_contract_address(lua_State *L, const char *address_str) = 0;
			virtual const char *get_system_asset_symbol(lua_State *L) = 0;
			virtual uint64_t get_system_asset_precision(lua_State *L) = 0;

			virtual std::vector<unsigned char> hex_to_bytes(const std::string& hex_string) = 0;
			virtual std::string bytes_to_hex(std::vector<unsigned char> bytes) = 0;
			virtual std::string sha256_hex(const std::string& hex_string) = 0;
			virtual std::string sha1_hex(const std::string& hex_string) = 0;
			virtual std::string sha3_hex(const std::string& hex_string) = 0;
			virtual std::string ripemd160_hex(const std::string& hex_string) = 0;

			virtual std::string get_address_role(lua_State* L, const std::string& addr) = 0;

			// when fork height < 0, not has this fork
			virtual int64_t get_fork_height(lua_State* L, const std::string& fork_key) = 0;

			// whether use cbor diff in storage diff
			virtual bool use_cbor_diff(lua_State* L) const = 0;

			virtual bool use_fast_map_set_nil(lua_State *L) const {
				return use_cbor_diff(L);
			}

			virtual std::string pubkey_to_address_string(const fc::ecc::public_key& pub) const = 0;

			virtual bool use_gas_log(lua_State* L) const {
				return false;
			}
			virtual bool use_step_log(lua_State* L) const {
				return false;
			}

			// invoked before contract execution
			virtual void before_contract_invoke(lua_State* L, const std::string& contract_addr, const std::string& txid) {}
			// dump contract's balances, storages to json
			virtual void dump_contract_state(lua_State* L, const std::string& contract_addr, const std::string& txid, std::ostream& out) {}

          };


          extern IUvmChainApi *global_uvm_chain_api;

        }
    }
}


#endif
