#include <simplechain/evaluate_state.h>
#include <simplechain/transaction.h>
#include <simplechain/blockchain.h>
#include <iostream>

#include <simplechain/native_contract.h>

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
		// TODO: read from parent evaluate_state first
		auto it1 = invoke_contract_result.storage_changes.find(contract_address);
		if (it1 != invoke_contract_result.storage_changes.end()) {
			auto& changes = it1->second;
			auto it2 = changes.find(key);
			if (it2 != changes.end()) {
				auto& change = *it2;
				return change.second;
			}
		}
		return chain->get_storage(contract_address, key);
	}

	std::map<std::string, StorageDataType> evaluate_state::get_contract_storages(const std::string& contract_addr) const {
		return chain->get_contract_storages(contract_addr);
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
		for (auto& p : invoke_contract_result.new_contracts) {
			if (p.first == contract_address) {
				p.second = contract_obj;
				return;
			}
		}
		invoke_contract_result.new_contracts.push_back(std::make_pair(contract_address, contract_obj));
	}

	bool evaluate_state::contains_contract_by_address(const std::string& contract_address) const {
		for (auto& p : invoke_contract_result.new_contracts) {
			if (p.first == contract_address) {
				return true;
			}
		}
		return chain->contains_contract_by_address(contract_address);
	}
	bool evaluate_state::contains_contract_by_name(const std::string& name) const {
		for (auto& p : invoke_contract_result.new_contracts) {
			if (p.second.contract_name == name) {
				return true;
			}
		}
		return chain->contains_contract_by_name(name);
	}

	std::shared_ptr<contract_object> evaluate_state::get_contract_by_address(const std::string& contract_address) const {
		for (auto& p : invoke_contract_result.new_contracts) {
			if (p.first == contract_address) {
				return std::make_shared<contract_object>(p.second);
			}
		}
		return chain->get_contract_by_address(contract_address);
	}

	std::shared_ptr<contract_object> evaluate_state::get_contract_by_name(const std::string& name) const {
		for (auto& p : invoke_contract_result.new_contracts) {
			if (p.second.contract_name == name) {
				return std::make_shared<contract_object>(p.second);
			}
		}
		return chain->get_contract_by_name(name);
	}

	void evaluate_state::update_account_asset_balance(const std::string& account_address, asset_id_t asset_id, int64_t balance_change) {
		for (auto& p : invoke_contract_result.account_balances_changes) {
			if (p.first.first == account_address && p.first.second == asset_id) {
				p.second += balance_change;
				return;
			}
		}
		auto key = std::make_pair(account_address, asset_id);
		invoke_contract_result.account_balances_changes[key] = balance_change;
	}

	share_type evaluate_state::get_account_asset_balance(const std::string& account_address, asset_id_t asset_id) const {
		auto balance = amount_change_type(chain->get_account_asset_balance(account_address, asset_id));
		for (const auto& p : invoke_contract_result.account_balances_changes) {
			balance += p.second;
		}
		// TODO: use parent evaluate_state's cache to update balance query result
		return share_type(balance);
	}

	void evaluate_state::set_contract_storage_changes(const std::string& contract_address, const contract_storage_changes_type& changes) {
		invoke_contract_result.storage_changes[contract_address] = changes;
	}

	cbor::CborObjectP evaluate_state::call_contract_api(const std::string& contractAddr, const std::string& apiName, cbor::CborArrayValue& args) {
		auto contract = get_contract_by_address(contractAddr);
		if (!contract) {
			throw uvm::core::UvmException(std::string("Can't find contract by address ") + contractAddr);
		}
		if (std::find(uvm::lua::lib::contract_special_api_names.begin(), uvm::lua::lib::contract_special_api_names.end(), apiName) != uvm::lua::lib::contract_special_api_names.end()) {
			throw uvm::core::UvmException(std::string("can't call special api:") + apiName + " directly");
		}

		if (contract->native_contract_key.empty()) {
			return engine->call_uvm_contract_api(contractAddr, apiName, args);
		}
		else { //native contract
			auto native_contract = native_contract_finder::create_native_contract_by_key(this, contract->native_contract_key, contractAddr);
			FC_ASSERT(native_contract, "native contract with the key not found");

			// TODO: ������arr
			std::string apiArg = "";
			if (args.size() > 0) {
				if(args.at(0)->is_string())
					apiArg = args.at(0)->as_string();
			}
			native_contract->invoke(apiName, apiArg);
			return cbor::CborObject::from_string(native_contract->get_api_result());
		}
	}

	bool evaluate_state::is_contract_has_api(const std::string &contract_address, const std::string &api_name) {
		auto contract = get_contract_by_address(contract_address);
		if (contract) {
			if (contract->native_contract_key.empty()) {
				if (contract->code.abi.find(api_name) != contract->code.abi.end()) {
					return true;
				}
				return false;
			}
			else { //native contract
				auto native_contract = native_contract_finder::create_native_contract_by_key(this, contract->native_contract_key, contract_address);
				auto &apis = native_contract->apis();
				if (apis.find(api_name) != apis.end()) {
					return true;
				}
				return false;
			}
		}
		return false;
	}

	static void merge_other_changes_to_evaluator(evaluate_state* evaluator, contract_invoke_result *_contract_invoke_resultp) {
		//merge result

		auto target_invoke_contract_resultp = &(evaluator->invoke_contract_result);
		//>>>>>>>>>>>>>>>...................................
		auto &target_account_balances_changes = target_invoke_contract_resultp->account_balances_changes;
		for (auto& p : _contract_invoke_resultp->account_balances_changes) {
			if (target_account_balances_changes.find(p.first) == target_account_balances_changes.end()) {
				target_account_balances_changes[p.first] = p.second;
			}
			else {
				target_account_balances_changes[p.first] += p.second;
			}
		}

		auto &target_events = target_invoke_contract_resultp->events;
		for (auto& event : _contract_invoke_resultp->events) {
			target_events.push_back(event);
		}

		auto &target_transfer_fees = target_invoke_contract_resultp->transfer_fees;
		for (auto& p : _contract_invoke_resultp->transfer_fees) {
			if (target_transfer_fees.find(p.first) == target_transfer_fees.end()) {
				target_transfer_fees[p.first] = p.second;
			}
			else {
				target_transfer_fees[p.first] += p.second;
			}
		}
		//target_invoke_contract_resultp->gas_used += _contract_invoke_resultp->gas_used;
	}

	void evaluate_state::commit_invoked_native_storage_changes() {
		int native_total_gas = 0;
		int native_total_exe_gas = 0;
		//calculate native contract storage and events gas
		contract_invoke_result native_contract_invoke_result;
		if ((gas_limit>0) && (invoked_native_contracts.size()> 0)) {
			auto iter =invoked_native_contracts.begin();
			while (iter != invoked_native_contracts.end()) {
				auto s_resultp = static_cast<contract_invoke_result*>(iter->second->get_result());

				auto &native_contract_id = iter->first;
				auto schanges_size = s_resultp->storage_changes.size();
				if (schanges_size > 0) {
					if (schanges_size == 1 && s_resultp->storage_changes.find(native_contract_id) != s_resultp->storage_changes.end()) {
						set_contract_storage_changes(native_contract_id, s_resultp->storage_changes[native_contract_id]);
						native_contract_invoke_result.storage_changes[native_contract_id] = s_resultp->storage_changes[native_contract_id];
					}
					else {
						throw uvm::core::UvmException("native contract can't set other contract storage");
					}
				}
				merge_other_changes_to_evaluator(this, s_resultp);
				for (auto& event : s_resultp->events) {
					native_contract_invoke_result.events.push_back(event);
				}
				native_total_exe_gas += s_resultp->gas_used;
				iter++;
			}

			int64_t native_storage_gas = native_contract_invoke_result.count_storage_gas();
			int64_t native_event_gas = native_contract_invoke_result.count_event_gas();
			native_total_gas += native_storage_gas;
			native_total_gas += native_event_gas;
			native_total_gas += native_total_exe_gas;
			//total_gas += invoke_contract_result.gas_used; //add exec native contract api gas//?????

			invoke_contract_result.gas_used += native_total_gas;
			gas_used += native_total_gas; //add 
		}
		invoked_native_contracts.clear(); //clear to prevent calculate twice
	}

}