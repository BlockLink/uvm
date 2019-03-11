#pragma once
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <cborcpp/cbor.h>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>
#include <uvm/uvm_api.h>
#include <simplechain/contract.h>
#include <simplechain/storage.h>
#include <simplechain/evaluate_state.h>

namespace simplechain {

	class native_contract_interface
	{
	protected:
		evaluate_state* _evaluate;
		contract_invoke_result _contract_invoke_result;
	public:
		std::string contract_id;

		native_contract_interface(evaluate_state* evaluate, const std::string& _contract_id) : _evaluate(evaluate), contract_id(_contract_id) {}
		virtual ~native_contract_interface() {}

		// unique key to identify native contract
		virtual std::string contract_key() const = 0;
		virtual std::set<std::string> apis() const = 0;
		virtual std::set<std::string> offline_apis() const = 0;
		virtual std::set<std::string> events() const = 0;

		virtual contract_invoke_result invoke(const std::string& api_name, const std::string& api_arg) = 0;

		virtual uint64_t gas_count_for_api_invoke(const std::string& api_name) const
		{
			return 100; // now all native api call requires 100 gas count
		}
		bool has_api(const std::string& api_name) const;

		void set_contract_storage(const std::string& contract_address, const std::string& storage_name, const StorageDataType& value);
		void set_contract_storage(const std::string& contract_address, const std::string& storage_name, cbor::CborObjectP cbor_value);
		void fast_map_set(const std::string& contract_address, const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value);
		StorageDataType get_contract_storage(const std::string& contract_address, const std::string& storage_name) const;
		cbor::CborObjectP get_contract_storage_cbor(const std::string& contract_address, const std::string& storage_name) const;
		cbor::CborObjectP fast_map_get(const std::string& contract_address, const std::string& storage_name, const std::string& key) const;
		std::string get_string_contract_storage(const std::string& contract_address, const std::string& storage_name) const;
		int64_t get_int_contract_storage(const std::string& contract_address, const std::string& storage_name) const;

		void emit_event(const std::string& contract_address, const std::string& event_name, const std::string& event_arg);

		uint64_t head_block_num() const;
		std::string caller_address_string() const;
		address caller_address() const;
		void throw_error(const std::string& err) const;

		void add_gas(uint64_t gas);
	};

	// this is native contract for token
	class token_native_contract : public native_contract_interface
	{
	public:
		static std::string native_contract_key() { return "token"; }

		token_native_contract(evaluate_state* evaluate, const address& _contract_id) : native_contract_interface(evaluate, _contract_id) {}
		virtual ~token_native_contract() {}

		virtual std::string contract_key() const;
		virtual address contract_address() const;
		virtual std::set<std::string> apis() const;
		virtual std::set<std::string> offline_apis() const;
		virtual std::set<std::string> events() const;

		virtual contract_invoke_result invoke(const std::string& api_name, const std::string& api_arg);
		std::string check_admin();
		std::string get_storage_state();
		std::string get_storage_token_name();
		std::string get_storage_token_symbol();
		int64_t get_storage_supply();
		int64_t get_storage_precision();
		// cbor::CborMapValue get_storage_users();
		// cbor::CborMapValue get_storage_allowed();
		int64_t get_balance_of_user(const std::string& owner_addr) const;
		cbor::CborMapValue get_allowed_of_user(const std::string& from_addr) const;
		std::string get_from_address();

		contract_invoke_result init_api(const std::string& api_name, const std::string& api_arg);
		contract_invoke_result init_token_api(const std::string& api_name, const std::string& api_arg);
		contract_invoke_result transfer_api(const std::string& api_name, const std::string& api_arg);
		contract_invoke_result balance_of_api(const std::string& api_name, const std::string& api_arg);
		contract_invoke_result state_api(const std::string& api_name, const std::string& api_arg);
		contract_invoke_result token_name_api(const std::string& api_name, const std::string& api_arg);
		contract_invoke_result token_symbol_api(const std::string& api_name, const std::string& api_arg);
		contract_invoke_result supply_api(const std::string& api_name, const std::string& api_arg);
		contract_invoke_result precision_api(const std::string& api_name, const std::string& api_arg);
		// 授权另一个用户可以从自己的余额中提现
		// arg format : spenderAddress, amount(with precision)
		contract_invoke_result approve_api(const std::string& api_name, const std::string& api_arg);
		// spender用户从授权人授权的金额中发起转账
		// arg format : fromAddress, toAddress, amount(with precision)
		contract_invoke_result transfer_from_api(const std::string& api_name, const std::string& api_arg);
		// 查询一个用户被另外某个用户授权的金额
		// arg format : spenderAddress, authorizerAddress
		contract_invoke_result approved_balance_from_api(const std::string& api_name, const std::string& api_arg);
		// 查询用户授权给其他人的所有金额
		// arg format : fromAddress
		contract_invoke_result all_approved_from_user_api(const std::string& api_name, const std::string& api_arg);

	};

	class native_contract_finder
	{
	public:
		static bool has_native_contract_with_key(const std::string& key);
		static std::shared_ptr<native_contract_interface> create_native_contract_by_key(evaluate_state* evaluate, const std::string& key, const address& contract_address);

	};


}