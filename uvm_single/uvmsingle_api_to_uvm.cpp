#include <uvm/uvm_api.h>
#include <uvm/uvm_lib.h>
#include <uvm/lauxlib.h>
#include <uvm/lapi.h>
#include <list>

#include <uvm/exceptions.h>

#include <fc/crypto/hex.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/elliptic.hpp>

#include <boost/algorithm/hex.hpp>

using namespace uvm::lua::lib;

namespace chain {
	// transfer from contract to account
	static int transfer_from_contract_to_public_account(lua_State *L)
	{
		if (lua_gettop(L) < 3)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "transfer_from_contract_to_public_account need 3 arguments");
			return 0;
		}
		const char *contract_id = get_storage_contract_id_in_api(L);
		if (nullptr == contract_id)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "contract transfer must be called in contract api");
			return 0;
		}
		const char *to_account_name = luaL_checkstring(L, 1);
		const char *asset_type = luaL_checkstring(L, 2);
		auto amount_str = luaL_checkinteger(L, 3);
		if (amount_str <= 0)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "amount must be positive");
			return 0;
		}
		lua_Integer transfer_result = uvm::lua::api::global_uvm_chain_api->transfer_from_contract_to_public_account(L, contract_id, to_account_name, asset_type, amount_str);
		lua_pushinteger(L, transfer_result);
		return 1;
	}

	/************************************************************************/
	/* transfer from contract to address                                    */
	/************************************************************************/
	static int transfer_from_contract_to_address(lua_State *L)
	{
		if (lua_gettop(L) < 3)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "transfer_from_contract_to_address need 3 arguments");
			return 0;
		}
		const char *contract_id = get_storage_contract_id_in_api(L);
		if (!contract_id)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "contract transfer must be called in contract api");
			return 0;
		}
		const char *to_address = luaL_checkstring(L, 1);
		const char *asset_type = luaL_checkstring(L, 2);
		auto amount_str = luaL_checkinteger(L, 3);
		if (amount_str <= 0)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "amount must be positive");
			return 0;
		}
		lua_Integer transfer_result = uvm::lua::api::global_uvm_chain_api->transfer_from_contract_to_address(L, contract_id, to_address, asset_type, amount_str);
		lua_pushinteger(L, transfer_result);
		return 1;
	}


	static int get_system_asset_symbol(lua_State *L)
	{
		const char *system_asset_symbol = uvm::lua::api::global_uvm_chain_api->get_system_asset_symbol(L);
		lua_pushstring(L, system_asset_symbol);
		return 1;
	}

	static int get_system_asset_precision(lua_State *L)
	{
		auto precision = uvm::lua::api::global_uvm_chain_api->get_system_asset_precision(L);
		lua_pushinteger(L, precision);
		return 1;
	}

	static int get_contract_balance_amount(lua_State *L)
	{
		if (lua_gettop(L) > 0 && !lua_isstring(L, 1))
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR,
				"get_contract_balance_amount need 1 string argument of contract address");
			return 0;
		}

		auto contract_address = luaL_checkstring(L, 1);
		if (strlen(contract_address) < 1)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR,
				"contract address can't be empty");
			return 0;
		}

		if (lua_gettop(L) < 2 || !lua_isstring(L, 2))
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "get balance amount need asset symbol");
			return 0;
		}

		auto assert_symbol = luaL_checkstring(L, 2);

		auto result = uvm::lua::api::global_uvm_chain_api->get_contract_balance_amount(L, contract_address, assert_symbol);
		lua_pushinteger(L, result);
		return 1;
	}

	static int signature_recover(lua_State* L) {
		// signature_recover(sig_hex, raw_hex): public_key_hex_string
		if (lua_gettop(L) < 2 || !lua_isstring(L, 1) || !lua_isstring(L, 2)) {
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "signature_recover need accept 2 hex string arguments");
			L->force_stopping = true;
			return 0;
		}
		uvm::lua::lib::increment_lvm_instructions_executed_count(L, CHAIN_GLUA_API_EACH_INSTRUCTIONS_COUNT * 10 - 1);
		std::string sig_hex(luaL_checkstring(L, 1));
		std::string raw_hex(luaL_checkstring(L, 2));

		try {
			const auto& sig_bytes = uvm::lua::api::global_uvm_chain_api->hex_to_bytes(sig_hex);
			const auto& raw_bytes = uvm::lua::api::global_uvm_chain_api->hex_to_bytes(raw_hex);
			fc::ecc::compact_signature compact_sig;
			if (sig_bytes.size() > compact_sig.size())
				throw uvm::core::UvmException("invalid sig bytes size");
			if (raw_bytes.size() != 32)
				throw uvm::core::UvmException("raw bytes should be 32 bytes");
			memcpy(compact_sig.data, sig_bytes.data(), sig_bytes.size());
			fc::sha256 raw_bytes_as_digest((char*)raw_bytes.data(), raw_bytes.size());
			auto recoved_public_key = fc::ecc::public_key(compact_sig, raw_bytes_as_digest, false);
			if (!recoved_public_key.valid()) {
				throw uvm::core::UvmException("invalid signature");
			}
			const auto& public_key_chars = recoved_public_key.serialize();
			std::vector<unsigned char> public_key_bytes(public_key_chars.begin(), public_key_chars.end());
			const auto& public_key_hex = uvm::lua::api::global_uvm_chain_api->bytes_to_hex(public_key_bytes);
			lua_pushstring(L, public_key_hex.c_str());
			return 1;
		}
		catch (const std::exception& e) {
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR,
				e.what());
			return 0;
		}
		catch (...) {
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR,
				"error when signature_recover");
			return 0;
		}
	}

	static int get_address_role(lua_State *L) {
		if (lua_gettop(L) < 1 || !lua_isstring(L, 1)) {
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR,
				"get_address_role need 1 address string argument");
			return 0;
		}
		auto addr = luaL_checkstring(L, 1);
		if (!uvm::lua::api::global_uvm_chain_api->is_valid_address(L, addr)) {
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR,
				"get_address_role's first argument must be valid address format");
			return 0;
		}
		std::string address_role = uvm::lua::api::global_uvm_chain_api->get_address_role(L, addr);
		lua_pushstring(L, address_role.c_str());
		return 1;
	}

	static int pubkey_to_address(lua_State* L) {
		try {
			if (lua_gettop(L) < 1 || !lua_isstring(L, 1)) {
				uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR,
					"pubkey_to_address need 1 pubkey hex string argument");
				return 0;
			}
			std::string pubkey_hex(luaL_checkstring(L, 1));
			std::vector<char> pubkey_chars;
			try {
				boost::algorithm::unhex(pubkey_hex, std::inserter(pubkey_chars, pubkey_chars.begin()));
			}
			catch (...) {
				throw uvm::core::UvmException("invalid pubkey hex");
			}
			fc::ecc::public_key_data pubkey_data;
			if (pubkey_chars.size() != pubkey_data.size()) {
				throw uvm::core::UvmException("invalid pubkey hex length");
			}
			memcpy(pubkey_data.data, pubkey_chars.data(), pubkey_data.size());
			fc::ecc::public_key pubkey(pubkey_data);
			auto addr = uvm::lua::api::global_uvm_chain_api->pubkey_to_address_string(pubkey);
			lua_pushstring(L, addr.c_str());
			return 1;
		}
		catch (const std::exception& e) {
			auto msg = std::string("pubkey_to_address error ") + e.what();
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, msg.c_str());
			return 0;
		}
		catch (const fc::exception& e) {
			auto msg = std::string("pubkey_to_address error ") + e.to_detail_string();
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, msg.c_str());
			return 0;
		}
		catch (...) {
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "pubkey_to_address error");
			return 0;
		}
	}


	static int get_chain_now(lua_State *L)
	{
		auto time = uvm::lua::api::global_uvm_chain_api->get_chain_now(L);
		lua_pushinteger(L, time);
		return 1;
	}

	static int get_chain_random(lua_State *L)
	{
		auto rand = uvm::lua::api::global_uvm_chain_api->get_chain_random(L);
		lua_pushinteger(L, rand);
		return 1;
	}

	static int get_chain_safe_random(lua_State *L)
	{
		bool diff_in_diff_txs = false;
		if (lua_gettop(L) >= 1 && lua_isboolean(L, 1) && lua_toboolean(L, 1)) {
			diff_in_diff_txs = true;
		}
		auto rand = uvm::lua::api::global_uvm_chain_api->get_chain_safe_random(L, diff_in_diff_txs);
		lua_pushinteger(L, rand);
		return 1;
	}

	static int get_transaction_id(lua_State *L)
	{
		std::string tid = uvm::lua::api::global_uvm_chain_api->get_transaction_id(L);
		lua_pushstring(L, tid.c_str());
		return 1;
	}
	static int get_transaction_fee(lua_State *L)
	{
		int64_t res = uvm::lua::api::global_uvm_chain_api->get_transaction_fee(L);
		lua_pushinteger(L, res);
		return 1;
	}
	static int get_header_block_num(lua_State *L)
	{
		auto result = uvm::lua::api::global_uvm_chain_api->get_header_block_num(L);
		lua_pushinteger(L, result);
		return 1;
	}

	static int wait_for_future_random(lua_State *L)
	{
		if (lua_gettop(L) < 1 || !lua_isinteger(L, 1))
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "wait_for_future_random need a integer param");
			return 0;
		}
		auto next = luaL_checkinteger(L, 1);
		if (next <= 0)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "wait_for_future_random first param must be positive number");
			return 0;
		}
		auto result = uvm::lua::api::global_uvm_chain_api->wait_for_future_random(L, (int)next);
		lua_pushinteger(L, result);
		return 1;
	}

	//lock_contract_balance_to_miner(symbol,amount,miner_id)
	static int lock_contract_balance_to_miner(lua_State *L)
	{
		if (lua_gettop(L) < 3)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "lock_contract_balance_to_miner need 4 arguments");
			return 0;
		}
		const char *contract_id = get_storage_contract_id_in_api(L);
		if (!contract_id)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "lock_contract_balance_to_miner must be called in contract api");
			return 0;
		}
		const char *asset_sym = luaL_checkstring(L, 1);
		const char *asset_amount = luaL_checkstring(L, 2);
		auto mid = luaL_checkstring(L, 3);
		int transfer_result = uvm::lua::api::global_uvm_chain_api->lock_contract_balance_to_miner(L, contract_id, asset_sym, asset_amount, mid);
		lua_pushboolean(L, transfer_result);
		return 1;
	}
	//obtain_pay_back_balance(miner_id,asset_sym,asset_amount)
	static int obtain_pay_back_balance(lua_State *L)
	{
		if (lua_gettop(L) < 3)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "obtain_pay_back_balance need 4 arguments");
			return 0;
		}
		const char *contract_id = get_storage_contract_id_in_api(L);
		if (!contract_id)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "obtain_pay_back_balance must be called in contract api");
			return 0;
		}

		auto mid = luaL_checkstring(L, 1);
		const char *asset_sym = luaL_checkstring(L, 2);
		const char *asset_amount = luaL_checkstring(L, 3);
		int transfer_result = uvm::lua::api::global_uvm_chain_api->obtain_pay_back_balance(L, contract_id, mid, asset_sym, asset_amount);
		lua_pushboolean(L, transfer_result);
		return 1;
	}
	//foreclose_balance_from_miners(miner_id,asset_sym,asset_amount)
	static int foreclose_balance_from_miners(lua_State *L)
	{
		if (lua_gettop(L) < 3)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "foreclose_balance_from_miners need 4 arguments");
			return 0;
		}
		const char *contract_id = get_storage_contract_id_in_api(L);
		if (!contract_id)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "foreclose_balance_from_miners must be called in contract api");
			return 0;
		}

		auto mid = luaL_checkstring(L, 1);
		const char *asset_sym = luaL_checkstring(L, 2);
		const char *asset_amount = luaL_checkstring(L, 3);
		int transfer_result = uvm::lua::api::global_uvm_chain_api->foreclose_balance_from_miners(L, contract_id, mid, asset_sym, asset_amount);
		lua_pushboolean(L, transfer_result);
		return 1;
	}
	//get_contract_lock_balance_info()
	static int get_contract_lock_balance_info(lua_State *L)
	{
		const char *contract_id = get_storage_contract_id_in_api(L);
		if (!contract_id)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "get_contract_lock_balance_info must be called in contract api");
			return 0;
		}
		std::string res = uvm::lua::api::global_uvm_chain_api->get_contract_lock_balance_info(L, contract_id);
		lua_pushstring(L, res.c_str());
		return 1;
	}
	//get_contract_lock_balance_info_by_asset(asset_sym: string)
	static int get_contract_lock_balance_info_by_asset(lua_State *L)
	{
		if (lua_gettop(L) < 1)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "get_contract_lock_balance_info_by_asset must be called in contract api");
			return 0;
		}
		const char *asset_sym = luaL_checkstring(L, 1);
		const char *contract_id = get_storage_contract_id_in_api(L);

		if (!contract_id)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "get_contract_lock_balance_info must be called in contract api");
			return 0;
		}
		std::string res = uvm::lua::api::global_uvm_chain_api->get_contract_lock_balance_info(L, contract_id, asset_sym);
		lua_pushstring(L, res.c_str());
		return 1;
	}
	//get_pay_back_balance(symbol)
	static int get_pay_back_balance(lua_State *L)
	{
		if (lua_gettop(L) < 1)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "get_pay_back_balance must be called in contract api");
			return 0;
		}
		const char *asset_sym = luaL_checkstring(L, 1);
		const char *contract_id = get_storage_contract_id_in_api(L);

		if (!contract_id)
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "get_pay_back_balance must be called in contract api");
			return 0;
		}
		std::string res = uvm::lua::api::global_uvm_chain_api->get_pay_back_balance(L, contract_id, asset_sym);
		lua_pushstring(L, res.c_str());
		return 1;
	}

	/**
	* get pseudo random number generate by some block(maybe future block or past block's data
	*/
	static int get_waited_block_random(lua_State *L)
	{
		if (lua_gettop(L) < 1 || !lua_isinteger(L, 1))
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "get_waited need a integer param");
			return 0;
		}
		auto num = luaL_checkinteger(L, 1);
		auto result = uvm::lua::api::global_uvm_chain_api->get_waited(L, (uint32_t)num);
		lua_pushinteger(L, result);
		return 1;
	}

	static int emit_uvm_event(lua_State *L)
	{
		if (lua_gettop(L) < 2 && (!lua_isstring(L, 1) || !lua_isstring(L, 2)))
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "emit need 2 string params");
			return 0;
		}
		const char *contract_id = get_storage_contract_id_in_api(L);
		const char *event_name = luaL_checkstring(L, 1);
		const char *event_param = luaL_checkstring(L, 2);
		if (!contract_id || strlen(contract_id) < 1)
			return 0;
		uvm::lua::api::global_uvm_chain_api->emit(L, contract_id, event_name, event_param);
		return 0;
	}

	static int is_valid_address(lua_State *L)
	{
		if (lua_gettop(L) < 1 || !lua_isstring(L, 1))
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "is_valid_address need a param of address string");
			return 0;
		}
		auto address = luaL_checkstring(L, 1);
		auto result = uvm::lua::api::global_uvm_chain_api->is_valid_address(L, address);
		lua_pushboolean(L, result ? 1 : 0);
		return 1;
	}

	static int is_valid_contract_address(lua_State *L)
	{
		if (lua_gettop(L) < 1 || !lua_isstring(L, 1))
		{
			uvm::lua::api::global_uvm_chain_api->throw_exception(L, UVM_API_SIMPLE_ERROR, "is_valid_contract_address need a param of address string");
			return 0;
		}
		auto address = luaL_checkstring(L, 1);
		auto result = uvm::lua::api::global_uvm_chain_api->is_valid_contract_address(L, address);
		lua_pushboolean(L, result ? 1 : 0);
		return 1;
	}



	static std::list<std::string> api_white_list = {
		"transfer_from_contract_to_address", "transfer_from_contract_to_public_account",
		"get_contract_balance_amount", "get_chain_now", "get_chain_random","get_chain_safe_random",
		"get_transaction_id", "get_header_block_num", "wait_for_future_random", "get_waited", "get_transaction_fee",
		"emit", "is_valid_address", "is_valid_contract_address",
		"get_system_asset_symbol","get_system_asset_precision",
		"signature_recover", "get_address_role", "lock_contract_balance_to_miner", "obtain_pay_back_balance", "foreclose_balance_from_miners",
		"get_contract_lock_balance_info", "get_contract_lock_balance_info_by_asset", "get_pay_back_balance", "pubkey_to_address"
	};

	void register_chain_api_to_uvm() {
		using uvm::lua::api::chain_apis;
		chain_apis["transfer_from_contract_to_address"] = &transfer_from_contract_to_address;
		chain_apis["transfer_from_contract_to_public_account"] = &transfer_from_contract_to_public_account;
		chain_apis["get_contract_balance_amount"] = &get_contract_balance_amount;
		chain_apis["get_chain_now"] = &get_chain_now;
		chain_apis["get_chain_random"] = &get_chain_random;
		chain_apis["get_chain_safe_random"] = &get_chain_safe_random;
		chain_apis["get_transaction_id"] = &get_transaction_id;
		chain_apis["get_header_block_num"] = &get_header_block_num;
		chain_apis["wait_for_future_random"] = &wait_for_future_random;
		chain_apis["get_waited"] = &get_waited_block_random;
		chain_apis["get_transaction_fee"] = &get_transaction_fee;
		chain_apis["emit"] = &emit_uvm_event;
		chain_apis["is_valid_address"] = &is_valid_address;
		chain_apis["is_valid_contract_address"] = &is_valid_contract_address;
		chain_apis["get_system_asset_symbol"] = &get_system_asset_symbol;
		chain_apis["get_system_asset_precision"] = &get_system_asset_precision;
		chain_apis["signature_recover"] = &signature_recover;
		chain_apis["get_address_role"] = &get_address_role;
		chain_apis["lock_contract_balance_to_miner"] = &lock_contract_balance_to_miner;
		chain_apis["obtain_pay_back_balance"] = &obtain_pay_back_balance;
		chain_apis["foreclose_balance_from_miners"] = &foreclose_balance_from_miners;
		chain_apis["get_contract_lock_balance_info"] = &get_contract_lock_balance_info;
		chain_apis["get_contract_lock_balance_info_by_asset"] = &get_contract_lock_balance_info_by_asset;
		chain_apis["get_pay_back_balance"] = &get_pay_back_balance;
		chain_apis["pubkey_to_address"] = &pubkey_to_address;


		for (auto iter = api_white_list.begin(); iter != api_white_list.end(); iter++) {
			add_api_to_uvm_white_list(*iter);
		}
	}


}