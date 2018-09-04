#include <simplechain/contract.h>
#include <simplechain/contract_entry.h>
#include <simplechain/blockchain.h>
#include <fc/crypto/sha512.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/io/raw.hpp>

namespace simplechain {
	contract_create_operation::contract_create_operation()
	: type(operation_type_enum::CONTRACT_CREATE),
	  gas_price(0),
	  gas_limit(0) {}

	contract_create_operation::~contract_create_operation() {

	}
	std::string contract_create_operation::calculate_contract_id() const
	{
		std::string id;
		fc::sha256::encoder enc;
		FC_ASSERT((contract_code != uvm::blockchain::Code()));
		if (contract_code != uvm::blockchain::Code())
		{
			std::pair<uvm::blockchain::Code, fc::time_point_sec> info_to_digest(contract_code, op_time);
			//fc::raw::pack(enc, info_to_digest);
			int32_t a = 123;
			auto sss = fc::raw::pack(a);
			fc::raw::pack(enc, contract_code);
			fc::raw::pack(enc, op_time);
		}

		id = fc::ripemd160::hash(enc.result()).str();
		return id;
	}

	contract_invoke_operation::contract_invoke_operation()
	: type(operation_type_enum::CONTRACT_INVOKE),
	  gas_price(0),
	  gas_limit(0) {}

	contract_invoke_operation::~contract_invoke_operation() {

	}


	void contract_invoke_result::reset()
	{
		api_result.clear();
		storage_changes.clear();
		contract_withdraw.clear();
		contract_balances.clear();
		deposit_to_address.clear();
		deposit_contract.clear();
		transfer_fees.clear();
		events.clear();
		exec_succeed = true;
	}

	void contract_invoke_result::set_failed()
	{
		api_result.clear();
		storage_changes.clear();
		contract_withdraw.clear();
		contract_balances.clear();
		deposit_to_address.clear();
		deposit_contract.clear();
		transfer_fees.clear();
		events.clear();
		exec_succeed = false;
	}

	void contract_invoke_result::apply_pendings(blockchain* chain, const std::string& tx_id) {
		std::vector<contract_event_notify_info> tx_events;
		auto tx_receipt = chain->get_tx_receipt(tx_id);
		if (!tx_receipt) {
			tx_receipt = std::make_shared<transaction_receipt>();
			tx_receipt->tx_id = tx_id;
		}
		for (const auto& p : events) {
			tx_receipt->events.push_back(p);
		}
		for (const auto& p : deposit_contract) {
			const auto& addr = p.first.first;
			auto asset_id = p.first.second;
			auto amount = p.second;
			chain->update_account_asset_balance(addr, asset_id, amount);
		}
		for (const auto& p : contract_withdraw) {
			const auto& addr = p.first.first;
			auto asset_id = p.first.second;
			auto amount = p.second;
			chain->update_account_asset_balance(addr, asset_id, -int64_t(amount));
		}
		// TODO: withdraw from caller's balances
		for (const auto& p : storage_changes) {
			const auto& contract_address = p.first;
			const auto& changes = p.second;
			for (const auto& it : changes) {
				chain->set_storage(contract_address, it.first, it.second.after);
			}
		}
		chain->set_tx_receipt(tx_id, *tx_receipt);
	}

}
