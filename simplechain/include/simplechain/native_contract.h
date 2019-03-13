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

	class abstract_native_contract
	{
	protected:
		evaluate_state* _evaluate;
		contract_invoke_result _contract_invoke_result;
	public:
		std::string contract_id;

		abstract_native_contract() : _evaluate(nullptr), contract_id("") {}
		abstract_native_contract(evaluate_state* evaluate, const std::string& _contract_id) : _evaluate(evaluate), contract_id(_contract_id) {}
		virtual ~abstract_native_contract() {}

		void init_config(evaluate_state* evaluate, const std::string& _contract_id);

		// unique key to identify native contract
		virtual std::string contract_key() const = 0;
		virtual std::set<std::string> apis() const = 0;
		virtual std::set<std::string> offline_apis() const = 0;
		virtual std::set<std::string> events() const = 0;

		virtual contract_invoke_result invoke(const std::string& api_name, const std::string& api_arg) = 0;

		virtual address contract_address() const;
		virtual uint64_t gas_count_for_api_invoke(const std::string& api_name) const
		{
			return 100; // now all native api call requires 100 gas count
		}
		bool has_api(const std::string& api_name) const;

		void set_contract_storage(const address& contract_address, const std::string& storage_name, const StorageDataType& value);
		void set_contract_storage(const address& contract_address, const std::string& storage_name, cbor::CborObjectP cbor_value);
		void fast_map_set(const address& contract_address, const std::string& storage_name, const std::string& key, cbor::CborObjectP cbor_value);
		StorageDataType get_contract_storage(const address& contract_address, const std::string& storage_name) const;
		cbor::CborObjectP get_contract_storage_cbor(const address& contract_address, const std::string& storage_name) const;
		cbor::CborObjectP fast_map_get(const address& contract_address, const std::string& storage_name, const std::string& key) const;
		std::string get_string_contract_storage(const address& contract_address, const std::string& storage_name) const;
		int64_t get_int_contract_storage(const address& contract_address, const std::string& storage_name) const;

		void emit_event(const address& contract_address, const std::string& event_name, const std::string& event_arg);

		uint64_t head_block_num() const;
		std::string caller_address_string() const;
		address caller_address() const;
		void throw_error(const std::string& err) const;

		void add_gas(uint64_t gas);
		void set_invoke_result_caller();
	};

	class native_contract_finder
	{
	public:
		static bool has_native_contract_with_key(const std::string& key);
		static std::shared_ptr<abstract_native_contract> create_native_contract_by_key(evaluate_state* evaluate, const std::string& key, const address& contract_address);

	};


}