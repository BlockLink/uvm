#pragma once
#include <simplechain/native_contract.h>

namespace simplechain {
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
}