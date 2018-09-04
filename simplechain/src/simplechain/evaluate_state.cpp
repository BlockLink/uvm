#include <simplechain/evaluate_state.h>
#include <simplechain/transaction.h>
#include <simplechain/blockchain.h>
#include <iostream>

namespace simplechain {
	evaluate_state::evaluate_state(blockchain* chain_, transaction* tx_)
	: chain(chain_), current_tx(tx_) {
	}
	blockchain* evaluate_state::get_chain() const {
		return chain;
	}
	void evaluate_state::set_current_tx(transaction* tx) {
		this->current_tx = tx;
	}
	transaction* evaluate_state::get_current_tx() const {
		return current_tx;
	}
	StorageDataType evaluate_state::get_storage(const std::string& contract_address, const std::string& key) const {
		// TODO: read cache first
		return chain->get_storage(contract_address, key);
	}

	void evaluate_state::emit_event(const std::string& contract_address, const std::string& event_name, const std::string& event_arg) {
		contract_event_notify_info event_notify_info;
		event_notify_info.block_num = get_chain()->latest_block().block_number + 1;
		event_notify_info.caller_addr = caller_address;
		event_notify_info.contract_address = contract_address;
		event_notify_info.event_name = event_name;
		event_notify_info.event_arg = event_arg;
		invoke_contract_result.events.push_back(event_notify_info);
	}

	void evaluate_state::store_contract(const std::string& contract_address, const contract_object& contract_obj) {
		// TODO
		std::cout << "TODO: store_contract in pending" << std::endl;
		chain->store_contract(contract_address, contract_obj);
	}

	void evaluate_state::set_contract_storage_changes(const std::string& contract_address, const contract_storage_changes_type& changes) {
		invoke_contract_result.storage_changes[contract_address] = changes;
	}

}