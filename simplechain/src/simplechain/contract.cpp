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
			fc::raw::pack(enc, info_to_digest);
		}

		id = fc::ripemd160::hash(enc.result()).str();
		return std::string(SIMPLECHAIN_CONTRACT_ADDRESS_PREFIX) + id;
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
		account_balances_changes.clear();
		transfer_fees.clear();
		events.clear();
		new_contracts.clear();
		exec_succeed = true;
	}

	void contract_invoke_result::set_failed()
	{
		api_result.clear();
		storage_changes.clear();
		account_balances_changes.clear();
		transfer_fees.clear();
		events.clear();
		new_contracts.clear();
		exec_succeed = false;
	}

	void contract_invoke_result::apply_pendings(blockchain* chain, const std::string& tx_id) {
		std::vector<contract_event_notify_info> tx_events;
		auto tx_receipt = chain->get_tx_receipt(tx_id);
		if (!tx_receipt) {
			tx_receipt = std::make_shared<transaction_receipt>();
			tx_receipt->tx_id = tx_id;
		}
		for (const auto& item : new_contracts) {
			chain->store_contract(item.first, item.second);
		}
		for (const auto& p : events) {
			tx_receipt->events.push_back(p);
		}
		for (const auto& p : account_balances_changes) {
			auto& addr = p.first.first;
			auto asset_id = p.first.second;
			auto amount_change = p.second;
			chain->update_account_asset_balance(addr, asset_id, amount_change);
		}
		for (const auto& p : storage_changes) {
			const auto& contract_address = p.first;
			const auto& changes = p.second;
			for (const auto& it : changes) {
				chain->set_storage(contract_address, it.first, it.second.after);
			}
		}
		chain->set_tx_receipt(tx_id, *tx_receipt);
	}

	fc::mutable_variant_object contract_event_notify_info::to_json() const {
		fc::mutable_variant_object info;
		info["contract_address"] = contract_address;
		info["event_name"] = event_name;
		info["event_arg"] = event_arg;
		info["caller_addr"] = caller_addr;
		info["block_num"] = block_num;
		return info;
	}

	fc::mutable_variant_object transaction_receipt::to_json() const {
		fc::mutable_variant_object info;
		info["tx_id"] = tx_id;
		fc::variants events_json;
		for (const auto& event_data : events) {
			events_json.push_back(event_data.to_json());
		}
		info["events"] = events_json;
		return info;
	}

}
