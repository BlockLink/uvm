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
	public:
		// unique key to identify native contract
		virtual std::string contract_key() const = 0;

		virtual std::set<std::string> apis() const = 0;
		virtual std::set<std::string> offline_apis() const = 0;
		virtual std::set<std::string> events() const = 0;

		// @throw std::exception
		virtual void invoke(const std::string& api_name, const std::string& api_arg) = 0;

		virtual uint64_t gas_count_for_api_invoke(const std::string& api_name) const {
			return get_proxy()->gas_count_for_api_invoke(api_name);
		}

		bool has_api(const std::string& api_name) const {
			const auto& api_names = apis();
			return api_names.find(api_name) != api_names.end();
		}

		virtual std::shared_ptr<native_contract_interface> get_proxy() const = 0;

		virtual void current_fast_map_set(const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value) {
			get_proxy()->current_fast_map_set(storage_name, key, cbor_value);
		}
		virtual cbor::CborObjectP get_current_contract_storage_cbor(const std::string& storage_name) const {
			return get_proxy()->get_current_contract_storage_cbor(storage_name);
		}
		virtual cbor::CborObjectP current_fast_map_get(const std::string& storage_name, const std::string& key) const {
			return get_proxy()->current_fast_map_get(storage_name, key);
		}
		virtual std::string get_string_current_contract_storage(const std::string& storage_name) const {
			return get_proxy()->get_string_current_contract_storage(storage_name);
		}
		virtual int64_t get_int_current_contract_storage(const std::string& storage_name) const {
			return get_proxy()->get_int_current_contract_storage(storage_name);
		}
		virtual void set_current_contract_storage(const std::string& storage_name, cbor::CborObjectP cbor_value) {
			get_proxy()->set_current_contract_storage(storage_name, cbor_value);
		}
		virtual void emit_event(const std::string& event_name, const std::string& event_arg) {
			get_proxy()->emit_event(event_name, event_arg);
		}

		virtual uint64_t head_block_num() const {
			return get_proxy()->head_block_num();
		}
		virtual std::string caller_address_string() const {
			return get_proxy()->caller_address_string();
		}
		virtual void throw_error(const std::string& err) const {
			get_proxy()->throw_error(err);
		}

		virtual void add_gas(uint64_t gas) {
			get_proxy()->add_gas(gas);
		}
		virtual void set_invoke_result_caller() {
			get_proxy()->set_invoke_result_caller();
		}
		// @return contract_invoke_result*
		virtual void* get_result() {
			return get_proxy()->get_result();
		}
		virtual void set_api_result(const std::string& api_result) {
			get_proxy()->set_api_result(api_result);
		}
	};

	class native_contract_store : public native_contract_interface
	{
	protected:
		evaluate_state* _evaluate;
		contract_invoke_result _contract_invoke_result;
	public:
		std::string contract_id;

		native_contract_store() : _evaluate(nullptr), contract_id("") {}
		native_contract_store(evaluate_state* evaluate, const std::string& _contract_id) : _evaluate(evaluate), contract_id(_contract_id) {}
		virtual ~native_contract_store() {}

		void init_config(evaluate_state* evaluate, const std::string& _contract_id);

		virtual std::shared_ptr<native_contract_interface> get_proxy() const {
			return nullptr;
		}

		virtual std::string contract_key() const {
			return "abstract";
		}

		virtual std::set<std::string> apis() const {
			return std::set<std::string>{};
		}
		virtual std::set<std::string> offline_apis() const {
			return std::set<std::string>{};
		}
		virtual std::set<std::string> events() const {
			return std::set<std::string>{};
		}

		virtual void invoke(const std::string& api_name, const std::string& api_arg) {
			
		}

		virtual address contract_address() const;
		virtual uint64_t gas_count_for_api_invoke(const std::string& api_name) const
		{
			return 100; // now all native api call requires 100 gas count
		}

		virtual void set_contract_storage(const address& contract_address, const std::string& storage_name, const StorageDataType& value);
		virtual void set_contract_storage(const address& contract_address, const std::string& storage_name, cbor::CborObjectP cbor_value);
		virtual void fast_map_set(const address& contract_address, const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value);
		virtual StorageDataType get_contract_storage(const address& contract_address, const std::string& storage_name) const;
		virtual cbor::CborObjectP get_contract_storage_cbor(const address& contract_address, const std::string& storage_name) const;
		virtual cbor::CborObjectP fast_map_get(const address& contract_address, const std::string& storage_name, const std::string& key) const;
		virtual std::string get_string_contract_storage(const address& contract_address, const std::string& storage_name) const;
		virtual int64_t get_int_contract_storage(const address& contract_address, const std::string& storage_name) const;

		virtual void emit_event(const address& contract_address, const std::string& event_name, const std::string& event_arg);

		virtual void current_fast_map_set(const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value) {
			fast_map_set(contract_address(), storage_name, key, cbor_value);
		}
		virtual StorageDataType get_current_contract_storage(const std::string& storage_name) const {
			return get_contract_storage(contract_address(), storage_name);
		}
		virtual cbor::CborObjectP get_current_contract_storage_cbor(const std::string& storage_name) const {
			return get_contract_storage_cbor(contract_address(), storage_name);
		}
		virtual cbor::CborObjectP current_fast_map_get(const std::string& storage_name, const std::string& key) const {
			return fast_map_get(contract_address(), storage_name, key);
		}
		virtual std::string get_string_current_contract_storage(const std::string& storage_name) const {
			return get_string_contract_storage(contract_address(), storage_name);
		}
		virtual int64_t get_int_current_contract_storage(const std::string& storage_name) const {
			return get_int_contract_storage(contract_address(), storage_name);
		}
		virtual void set_current_contract_storage(const std::string& storage_name, cbor::CborObjectP cbor_value) {
			set_contract_storage(contract_address(), storage_name, cbor_value);
		}
		virtual void set_current_contract_storage(const std::string& storage_name, const StorageDataType& value) {
			set_contract_storage(contract_address(), storage_name, value);
		}
		virtual void emit_event(const std::string& event_name, const std::string& event_arg) {
			emit_event(contract_address(), event_name, event_arg);
		}

		virtual uint64_t head_block_num() const;
		virtual std::string caller_address_string() const;
		virtual address caller_address() const;
		virtual void throw_error(const std::string& err) const;

		virtual void add_gas(uint64_t gas);
		virtual void set_invoke_result_caller();

		virtual void* get_result() { return &_contract_invoke_result; }
		virtual void set_api_result(const std::string& api_result) {
			_contract_invoke_result.api_result = api_result;
		}
	};

	class native_contract_finder
	{
	public:
		static bool has_native_contract_with_key(const std::string& key);
		static std::shared_ptr<native_contract_interface> create_native_contract_by_key(evaluate_state* evaluate, const std::string& key, const address& contract_address);

	};


}