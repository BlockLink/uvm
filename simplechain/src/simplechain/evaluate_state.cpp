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
		// TODO
		std::cout << "emited event in contract " << contract_address << " event " << event_name << " arg " << event_arg << std::endl;
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