#include <simplechain/contract_evaluate.h>
#include <simplechain/contract_engine_builder.h>
#include <simplechain/contract_object.h>
#include <simplechain/blockchain.h>
#include <simplechain/address_helper.h>
#include <simplechain/native_contract.h>
#include <iostream>
#include <uvm/uvm_lib.h>
#include <fc/io/json.hpp>

namespace simplechain {
	using namespace std;
	// contract_create_evaluator methods
	std::shared_ptr<contract_create_evaluator::operation_type::result_type> contract_create_evaluator::do_evaluate(const operation_type& o) {
		ContractEngineBuilder builder;
		auto engine = builder.build();
		int exception_code = 0;
		string exception_msg;
		bool has_error = false;
		try {
			auto origin_op = o;
			engine->set_caller(o.caller_address, o.caller_address);
			engine->set_state_pointer_value("register_evaluate_state", this);
			engine->clear_exceptions();
			auto limit = o.gas_limit;
			if (limit < 0 || limit == 0)
				throw uvm::core::UvmException("invalid_contract_gas_limit");
			gas_limit = limit;
			engine->set_gas_limit(limit);
			invoke_contract_result.reset();
			std::string contract_address = o.calculate_contract_id();
			contract_object contract;
			contract.code = o.contract_code;
			contract.contract_address = contract_address;
			contract.owner_address = o.caller_address;
			contract.create_time = o.op_time;
			contract.registered_block = get_chain()->latest_block().block_number + 1;
			contract.type_of_contract = contract_type::normal_contract;
			store_contract(contract_address, contract);
			try
			{
				std::string result_json_str;
				engine->execute_contract_init_by_address(contract_address, "", &result_json_str);
				invoke_contract_result.api_result = result_json_str;
			}
			catch (std::exception &e)
			{
				throw uvm::core::UvmException(e.what());
			}

			gas_used = engine->gas_used();
			FC_ASSERT(gas_used <= gas_limit && gas_used > 0, "costs of execution can be only between 0 and init_cost");

			auto gas_count = gas_used;
			invoke_contract_result.exec_succeed = true;
			invoke_contract_result.gas_used = gas_count;
		}
		catch (const std::exception& e)
		{
			has_error = true;
			undo_contract_effected();
			std::cerr << e.what() << std::endl;
			invoke_contract_result.error = e.what();
		}
		return std::make_shared<contract_invoke_result>(invoke_contract_result);
	}
	std::shared_ptr<contract_create_evaluator::operation_type::result_type> contract_create_evaluator::do_apply(const operation_type& op) {
		auto result = do_evaluate(op);
		result->apply_pendings(get_chain(), get_current_tx()->tx_hash());
		return result;
	}

	void contract_create_evaluator::undo_contract_effected()
	{
		invoke_contract_result.set_failed();
	}

	// native_contract_create_evaluator methods
	std::shared_ptr<native_contract_create_evaluator::operation_type::result_type> native_contract_create_evaluator::do_evaluate(const operation_type& o) {
		
		int exception_code = 0;
		string exception_msg;
		bool has_error = false;
		try {
			auto origin_op = o;
			auto limit = o.gas_limit;
			if (limit <= 0)
				throw uvm::core::UvmException("invalid_contract_gas_limit");
			gas_limit = limit;
			invoke_contract_result.reset();
			std::string contract_address = o.calculate_contract_id();
			contract_object contract;
			contract.native_contract_key = o.template_key;
			contract.contract_address = contract_address;
			contract.owner_address = o.caller_address;
			contract.create_time = o.op_time;
			contract.registered_block = get_chain()->latest_block().block_number + 1;
			contract.type_of_contract = contract_type::native_contract;

			this->caller_address = o.caller_address;

			auto native_contract = native_contract_finder::create_native_contract_by_key(this, o.template_key, contract_address);
			FC_ASSERT(native_contract);

			store_contract(contract_address, contract);
			try
			{
				native_contract->invoke("init", "");
				auto native_result = *native_contract->get_result();
				native_result.new_contracts = invoke_contract_result.new_contracts;
				invoke_contract_result = native_result;
			}
			catch (const std::exception &e)
			{
				throw uvm::core::UvmException(e.what());
			}

			gas_used = invoke_contract_result.gas_used;
			FC_ASSERT(gas_used <= gas_limit && gas_used > 0, "costs of execution can be only between 0 and init_cost");

			auto gas_count = gas_used;
			invoke_contract_result.exec_succeed = true;
			invoke_contract_result.gas_used = gas_count;
		}
		catch (const std::exception& e)
		{
			has_error = true;
			undo_contract_effected();
			std::cerr << e.what() << std::endl;
			invoke_contract_result.error = e.what();
		}
		return std::make_shared<contract_invoke_result>(invoke_contract_result);
	}
	std::shared_ptr<native_contract_create_evaluator::operation_type::result_type> native_contract_create_evaluator::do_apply(const operation_type& op) {
		auto result = do_evaluate(op);
		result->apply_pendings(get_chain(), get_current_tx()->tx_hash());
		return result;
	}

	void native_contract_create_evaluator::undo_contract_effected()
	{
		invoke_contract_result.set_failed();
	}

	// contract_invoke_evaluator methods
	std::shared_ptr<contract_invoke_evaluator::operation_type::result_type> contract_invoke_evaluator::do_evaluate(const operation_type& o) {
		int exception_code = 0;
		string exception_msg;
		bool has_error = false;
		try {
			FC_ASSERT(helper::is_valid_contract_address(o.contract_address), "invalid contract address");
			auto origin_op = o;
			auto limit = o.gas_limit;
			if (limit <= 0)
				throw uvm::core::UvmException("invalid_contract_gas_limit");
			gas_limit = limit;
			invoke_contract_result.reset();
			try
			{
				std::string first_contract_arg = o.contract_args.empty() ? "" : o.contract_args[0];
				// only can call on_deposit_asset when deposit_amount > 0
				if (o.deposit_amount > 0) {
					if (o.contract_api != "on_deposit_asset") {
						throw uvm::core::UvmException("only can deposit to contract by call api on_deposit_asset");
					}
					auto asset = chain->get_asset(o.deposit_asset_id);
					if(!asset) {
						throw uvm::core::UvmException(std::string("can't find asset #") + std::to_string(o.deposit_asset_id));
					}
					fc::mutable_variant_object depositArgs;
					depositArgs["num"] = o.deposit_amount;
					depositArgs["symbol"] = asset->symbol;
					depositArgs["param"] = first_contract_arg;
					first_contract_arg = fc::json::to_string(depositArgs);
				}
				else {
					if (std::find(uvm::lua::lib::contract_special_api_names.begin(), uvm::lua::lib::contract_special_api_names.end(), o.contract_api) != uvm::lua::lib::contract_special_api_names.end()) {
						throw uvm::core::UvmException(std::string("can't call ") + o.contract_api + " directly");
					}
				}
				std::string result_json_str;
				if (o.deposit_amount > 0) {
					update_account_asset_balance(o.contract_address, o.deposit_asset_id, o.deposit_amount);
				}
				auto contract = get_contract_by_address(o.contract_address);
				FC_ASSERT(contract, "Can't find contract by address");
				if (contract->native_contract_key.empty()) {
					ContractEngineBuilder builder;
					auto engine = builder.build();
					engine->set_caller(o.caller_address, o.caller_address);
					engine->set_state_pointer_value("invoke_evaluate_state", this);
					engine->clear_exceptions();
					engine->set_gas_limit(limit);
					engine->execute_contract_api_by_address(o.contract_address, o.contract_api, first_contract_arg, &result_json_str);
					invoke_contract_result.api_result = result_json_str;
					gas_used = engine->gas_used();
				}
				else {
					// native contract
					this->caller_address = o.caller_address;
					auto native_contract = native_contract_finder::create_native_contract_by_key(this, contract->native_contract_key, o.contract_address);
					FC_ASSERT(native_contract);
					native_contract->invoke(o.contract_api, first_contract_arg);
					invoke_contract_result = *native_contract->get_result();
					gas_used = invoke_contract_result.gas_used;
				}
			}
			catch (std::exception &e)
			{
				if (o.deposit_amount > 0) {
					update_account_asset_balance(o.contract_address, o.deposit_asset_id, (0 - (o.deposit_amount)));
				}
				throw uvm::core::UvmException(e.what());
			}

			FC_ASSERT(gas_used <= gas_limit && gas_used > 0, "costs of execution can be only between 0 and init_cost");

			auto gas_count = gas_used;
			invoke_contract_result.exec_succeed = true;
			invoke_contract_result.gas_used = gas_count;

		}
		catch (const std::exception& e)
		{
			has_error = true;
			undo_contract_effected();
			std::cerr << e.what() << std::endl;
			invoke_contract_result.error = e.what();
		}
		return std::make_shared<contract_invoke_result>(invoke_contract_result);
	}
	std::shared_ptr<contract_invoke_evaluator::operation_type::result_type> contract_invoke_evaluator::do_apply(const operation_type& op) {
		auto result = do_evaluate(op);
		result->apply_pendings(get_chain(), get_current_tx()->tx_hash());
		return result;
	}

	void contract_invoke_evaluator::undo_contract_effected()
	{
		invoke_contract_result.set_failed();
	}
}
