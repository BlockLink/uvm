#include <simplechain/contract_evaluate.h>
#include <simplechain/contract_engine_builder.h>
#include <simplechain/contract_object.h>
#include <simplechain/blockchain.h>

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
			contract.owner_address = caller_address;
			contract.create_time = o.op_time;
			contract.registered_block = get_chain()->latest_block().block_number + 1;
			contract.type_of_contract = contract_type::normal_contract;
			store_contract(contract_address, contract);
			try
			{
				engine->execute_contract_init_by_address(contract_address, "", nullptr);
			}
			catch (std::exception &e)
			{
				throw uvm::core::UvmException(e.what());
			}

			gas_used = engine->gas_used();
			FC_ASSERT(gas_used <= gas_limit && gas_used > 0, "costs of execution can be only between 0 and init_cost");

			auto gas_count = gas_used;
			contract_object new_contract;
			invoke_contract_result.exec_succeed = true;
		}
		catch (const std::exception& e)
		{
			has_error = true;
			undo_contract_effected();
		}
		return std::make_shared<contract_invoke_result>(invoke_contract_result);
	}
	std::shared_ptr<contract_create_evaluator::operation_type::result_type> contract_create_evaluator::do_apply(const operation_type& op) {
		auto result = do_evaluate(op);
		// TODO: apply events
		// update balances
		for (const auto& p : result->deposit_contract) {
			const auto& addr = p.first.first;
			auto asset_id = p.first.second;
			auto amount = p.second;
			get_chain()->update_account_asset_balance(addr, asset_id, amount);
		}
		for (const auto& p : result->contract_withdraw) {
			const auto& addr = p.first.first;
			auto asset_id = p.first.second;
			auto amount = p.second;
			get_chain()->update_account_asset_balance(addr, asset_id, -int64_t(amount));
		}
		// TODO: withdraw from caller's balances
		for (const auto& p : result->storage_changes) {
			const auto& contract_address = p.first;
			const auto& changes = p.second;
			for (const auto& it : changes) {
				get_chain()->set_storage(contract_address, it.first, it.second.after);
			}
		}
		return result;
	}

	void contract_create_evaluator::undo_contract_effected()
	{
		invoke_contract_result.set_failed();
	}

	// contract_invoke_evaluator methods
	std::shared_ptr<contract_invoke_evaluator::operation_type::result_type> contract_invoke_evaluator::do_evaluate(const operation_type& o) {
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
			try
			{
				engine->execute_contract_api_by_address(o.contract_address, o.contract_api, o.contract_args.empty() ? "" : o.contract_args[0], nullptr);
			}
			catch (std::exception &e)
			{
				throw uvm::core::UvmException(e.what());
			}

			gas_used = engine->gas_used();
			FC_ASSERT(gas_used <= gas_limit && gas_used > 0, "costs of execution can be only between 0 and init_cost");

			auto gas_count = gas_used;
			// new_contract.registered_block = d.head_block_num() + 1;
			invoke_contract_result.exec_succeed = true;
		}
		catch (const std::exception& e)
		{
			has_error = true;
			undo_contract_effected();
		}
		return std::make_shared<contract_invoke_result>(invoke_contract_result);
	}
	std::shared_ptr<contract_invoke_evaluator::operation_type::result_type> contract_invoke_evaluator::do_apply(const operation_type& op) {
		// TODO: duplicate code
		auto result = do_evaluate(op);
		// TODO: apply events
		// update balances
		for (const auto& p : result->deposit_contract) {
			const auto& addr = p.first.first;
			auto asset_id = p.first.second;
			auto amount = p.second;
			get_chain()->update_account_asset_balance(addr, asset_id, amount);
		}
		for (const auto& p : result->contract_withdraw) {
			const auto& addr = p.first.first;
			auto asset_id = p.first.second;
			auto amount = p.second;
			get_chain()->update_account_asset_balance(addr, asset_id, -int64_t(amount));
		}
		// TODO: withdraw from caller's balances
		for (const auto& p : result->storage_changes) {
			const auto& contract_address = p.first;
			const auto& changes = p.second;
			for (const auto& it : changes) {
				get_chain()->set_storage(contract_address, it.first, it.second.after);
			}
		}
		return result;
	}

	void contract_invoke_evaluator::undo_contract_effected()
	{
		invoke_contract_result.set_failed();
	}
}