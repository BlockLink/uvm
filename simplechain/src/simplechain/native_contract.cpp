#include <simplechain/native_contract.h>
#include <simplechain/native_token_contract.h>
#include <simplechain/blockchain.h>
#include <boost/algorithm/string.hpp>
#include <jsondiff/jsondiff.h>

namespace simplechain {

#define THROW_CONTRACT_ERROR(...) FC_ASSERT(false, __VA_ARGS__)

	using namespace cbor_diff;
	using namespace cbor;
	using namespace std;

	void abstract_native_contract::init_config(evaluate_state* evaluate, const std::string& _contract_id) {
		this->_evaluate = evaluate;
		this->contract_id = _contract_id;
	}

	bool abstract_native_contract::has_api(const std::string& api_name) const {
		const auto& api_names = apis();
		return api_names.find(api_name) != api_names.end();
	}

	void abstract_native_contract::set_contract_storage(const address& contract_address, const std::string& storage_name, const StorageDataType& value)
	{
		if (_contract_invoke_result.storage_changes.find(contract_address) == _contract_invoke_result.storage_changes.end())
		{
			_contract_invoke_result.storage_changes[contract_address] = contract_storage_changes_type();
		}
		auto& storage_changes = _contract_invoke_result.storage_changes[contract_address];
		if (storage_changes.find(storage_name) == storage_changes.end())
		{
			StorageDataChangeType change;
			change.after = value;
			const auto &before = _evaluate->get_storage(contract_address, storage_name);
			cbor_diff::CborDiff differ;
			const auto& before_cbor = cbor_decode(before.storage_data);
			const auto& after_cbor = cbor_decode(change.after.storage_data);
			auto diff = differ.diff(before_cbor, after_cbor);
			change.storage_diff.storage_data = cbor_encode(diff->value());
			change.before = before;
			storage_changes[storage_name] = change;
		}
		else
		{
			auto& change = storage_changes[storage_name];
			auto before = change.before;
			auto after = value;
			change.after = after;
			cbor_diff::CborDiff differ;
			const auto& before_cbor = cbor_diff::cbor_decode(before.storage_data);
			const auto& after_cbor = cbor_diff::cbor_decode(after.storage_data);
			auto diff = differ.diff(before_cbor, after_cbor);
			change.storage_diff.storage_data = cbor_encode(diff->value());
		}
	}

	void abstract_native_contract::set_contract_storage(const address& contract_address, const std::string& storage_name, cbor::CborObjectP cbor_value) {
		StorageDataType value;
		value.storage_data = cbor_encode(cbor_value);
		return set_contract_storage(contract_address, storage_name, value);
	}

	void abstract_native_contract::fast_map_set(const address& contract_address, const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value) {
		std::string full_key = storage_name + "." + key;
		set_contract_storage(contract_address, full_key, cbor_value);
	}

	StorageDataType abstract_native_contract::get_contract_storage(const address& contract_address, const std::string& storage_name) const
	{
		if (_contract_invoke_result.storage_changes.find(contract_address) == _contract_invoke_result.storage_changes.end())
		{
			return _evaluate->get_storage(contract_address, storage_name);
		}
		auto& storage_changes = _contract_invoke_result.storage_changes.at(contract_address);
		if (storage_changes.find(storage_name) == storage_changes.end())
		{
			return _evaluate->get_storage(contract_address, storage_name);
		}
		return storage_changes.at(storage_name).after;
	}

	cbor::CborObjectP abstract_native_contract::get_contract_storage_cbor(const address& contract_address, const std::string& storage_name) const {
		const auto& data = get_contract_storage(contract_address, storage_name);
		return cbor_decode(data.storage_data);
	}

	std::string abstract_native_contract::get_string_contract_storage(const address& contract_address, const std::string& storage_name) const {
		auto cbor_data = get_contract_storage_cbor(contract_address, storage_name);
		if (!cbor_data->is_string()) {
			throw_error(std::string("invalid string contract storage ") + contract_address + "." + storage_name);
		}
		return cbor_data->as_string();
	}

	cbor::CborObjectP abstract_native_contract::fast_map_get(const address& contract_address, const std::string& storage_name, const std::string& key) const {
		std::string full_key = storage_name + "." + key;
		return get_contract_storage_cbor(contract_address, full_key);
	}

	int64_t abstract_native_contract::get_int_contract_storage(const address& contract_address, const std::string& storage_name) const {
		auto cbor_data = get_contract_storage_cbor(contract_address, storage_name);
		if (!cbor_data->is_integer()) {
			throw_error(std::string("invalid int contract storage ") + contract_address + "." + storage_name);
		}
		return cbor_data->force_as_int();
	}

	void abstract_native_contract::emit_event(const address& contract_address, const std::string& event_name, const std::string& event_arg)
	{
		FC_ASSERT(!event_name.empty());
		contract_event_notify_info info;
		info.event_name = event_name;
		info.event_arg = event_arg;
		//info.caller_addr = caller_address->address_to_string();
		info.block_num = 1 + head_block_num();

		_contract_invoke_result.events.push_back(info);
	}

	uint64_t abstract_native_contract::head_block_num() const {
		return _evaluate->get_chain()->head_block_number();
	}

	std::string abstract_native_contract::caller_address_string() const {
		return _evaluate->caller_address;
	}

	address abstract_native_contract::caller_address() const {
		return _evaluate->caller_address;
	}

	address abstract_native_contract::contract_address() const {
		return contract_id;
	}

	void abstract_native_contract::throw_error(const std::string& err) const {
		FC_THROW_EXCEPTION(fc::assert_exception, err);
	}

	void abstract_native_contract::add_gas(uint64_t gas) {
		_contract_invoke_result.gas_used += gas;
	}

	void abstract_native_contract::set_invoke_result_caller() {
		_contract_invoke_result.invoker = caller_address();
	}

	// class native_contract_finder

	bool native_contract_finder::has_native_contract_with_key(const std::string& key)
	{
		std::vector<std::string> native_contract_keys = {
			// demo_native_contract::native_contract_key(),
			token_native_contract::native_contract_key()
		};
		return std::find(native_contract_keys.begin(), native_contract_keys.end(), key) != native_contract_keys.end();
	}
	shared_ptr<abstract_native_contract> native_contract_finder::create_native_contract_by_key(evaluate_state* evaluate, const std::string& key, const address& contract_address)
	{
		shared_ptr<abstract_native_contract> result;
		/*if (key == demo_native_contract::native_contract_key())
		{
			result = std::make_shared<demo_native_contract>();
		}
		else */
		if (key == token_native_contract::native_contract_key())
		{
			result = std::make_shared<token_native_contract>();
		}
		else
		{
			return nullptr;
		}
		if (result)
			result->init_config(evaluate, contract_address);
		return result;
	}


}