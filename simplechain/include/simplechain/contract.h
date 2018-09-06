#pragma once
#include <simplechain/operation.h>
#include <simplechain/evaluate_result.h>
#include <simplechain/contract_entry.h>
#include <simplechain/storage.h>
#include <simplechain/contract_object.h>
#include <simplechain/asset.h>

namespace simplechain {

	typedef std::string contract_address_type;
	typedef std::string address;
	typedef int64_t amount_change_type;

	struct contract_event_notify_info
	{
		contract_address_type contract_address;
		std::string event_name;
		std::string event_arg;
		std::string caller_addr;
		uint64_t block_num;
	};

	struct comparator_for_contract_invoke_result_balance {
		bool operator() (const std::pair<contract_address_type, asset_id_t>& x, const std::pair<contract_address_type, asset_id_t>& y) const
		{
			std::string x_addr_str = x.first;
			std::string y_addr_str = y.first;
			if (x_addr_str < y_addr_str) {
				return true;
			}
			if (x_addr_str > y_addr_str) {
				return false;
			}
			return x.second < y.second;
		}
	};

	struct comparator_for_contract_invoke_result_balance_change {
		bool operator() (const std::pair<address, amount_change_type>& x, const std::pair<address, amount_change_type>& y) const
		{
			std::string x_addr_str = x.first;
			std::string y_addr_str = y.first;
			if (x_addr_str < y_addr_str) {
				return true;
			}
			if (x_addr_str > y_addr_str) {
				return false;
			}
			return x.second < y.second;
		}
	};

	class blockchain;


	struct contract_invoke_result : public evaluate_result
	{
		std::string api_result;
		std::map<std::string, contract_storage_changes_type, std::less<std::string>> storage_changes;
		std::map<std::pair<address, asset_id_t>, amount_change_type, comparator_for_contract_invoke_result_balance_change> account_balances_changes;

		std::map<asset_id_t, share_type, std::less<asset_id_t>> transfer_fees;
		std::vector<contract_event_notify_info> events;
		std::vector<std::pair<contract_address_type, contract_object> > new_contracts;
		bool exec_succeed = true;
		address invoker;
		void reset();
		void set_failed();

		void apply_pendings(blockchain* chain, const std::string& tx_id);
	};



	struct transaction_receipt {
		std::string tx_id;
		std::vector<contract_event_notify_info> events;
	};


	// contract operations
	struct contract_create_operation : public generic_operation {
		typedef contract_invoke_result result_type;

		operation_type_enum type;
		std::string caller_address;
		uvm::blockchain::Code contract_code;
		uint64_t gas_price;
		uint64_t gas_limit;
		fc::time_point_sec op_time;

		contract_create_operation();
		virtual ~contract_create_operation();

		virtual operation_type_enum get_type() const {
			return type;
		}

		std::string calculate_contract_id() const;
	};

	struct contract_invoke_operation : public generic_operation {
		typedef contract_invoke_result result_type;

		operation_type_enum type;
		std::string caller_address;
		std::string contract_address;
		std::string contract_api;
		std::vector<std::string> contract_args;
		uint64_t gas_price;
		uint64_t gas_limit;
		fc::time_point_sec op_time;

		contract_invoke_operation();
		virtual ~contract_invoke_operation();

		virtual operation_type_enum get_type() const {
			return type;
		}
	};

}

FC_REFLECT(simplechain::transaction_receipt, (tx_id)(events))

FC_REFLECT(simplechain::contract_create_operation, (type)(caller_address)(contract_code)(gas_price)(gas_limit)(op_time))
FC_REFLECT(simplechain::contract_invoke_operation, (type)(caller_address)(contract_address)(contract_api)(contract_args)(gas_price)(gas_limit)(op_time))
