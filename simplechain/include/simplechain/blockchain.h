#pragma once
#include <simplechain/config.h>
#include <simplechain/transaction.h>
#include <simplechain/block.h>
#include <simplechain/evaluator.h>
#include <simplechain/evaluate_result.h>
#include <simplechain/evaluate_state.h>
#include <simplechain/contract_evaluate.h>
#include <simplechain/contract_object.h>
#include <simplechain/storage.h>
#include <simplechain/asset.h>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>

namespace simplechain {
	typedef int64_t balance_t; //int64

	class blockchain {
		// TODO: local db and rollback
	private:
		std::vector<asset> assets;
		std::vector<block> blocks;
		std::map<std::string, transaction_receipt> tx_receipts; // txid => tx_receipt
		std::map<std::string, std::map<asset_id_t, balance_t> > account_balances;
		std::map<std::string, contract_object> contracts;
		std::map<std::string, std::map<std::string, StorageDataType> > contract_storages;
		std::vector<transaction> tx_mempool;
	public:
		blockchain();
		// @throws exception
		std::shared_ptr<evaluate_result> evaluate_transaction(std::shared_ptr<transaction> tx);
		void apply_transaction(std::shared_ptr<transaction> tx);
		block latest_block() const;
		uint64_t head_block_number() const;
		std::string head_block_hash() const;
		std::shared_ptr<transaction> get_trx_by_hash(const std::string& tx_hash) const;
		std::shared_ptr<block> get_block_by_number(uint64_t num) const;
		std::shared_ptr<block> get_block_by_hash(const std::string& block_hash) const;
		balance_t get_account_asset_balance(const std::string& account_address, asset_id_t asset_id) const;
		std::map<asset_id_t, balance_t> get_account_balances(const std::string& account_address) const;
		void update_account_asset_balance(const std::string& account_address, asset_id_t asset_id, int64_t balance_change);
		std::shared_ptr<contract_object> get_contract_by_address(const std::string& addr) const;
		std::shared_ptr<contract_object> get_contract_by_name(const std::string& name) const;
		void store_contract(const std::string& addr, const contract_object& contract_obj);
		StorageDataType get_storage(const std::string& contract_address, const std::string& key) const;
		std::map<std::string, StorageDataType> get_contract_storages(const std::string& contract_address) const;
		void set_storage(const std::string& contract_address, const std::string& key, const StorageDataType& value);
		void add_asset(const asset& new_asset);
		std::shared_ptr<asset> get_asset(asset_id_t asset_id);
		std::shared_ptr<asset> get_asset_by_symbol(const std::string& symbol);
		std::vector<asset> get_assets() const;
		std::vector<contract_object> get_contracts() const;
		std::vector<std::string> get_account_addresses() const;
		void set_tx_receipt(const std::string& tx_id, const transaction_receipt& tx_receipt);
		std::shared_ptr<transaction_receipt> get_tx_receipt(const std::string& tx_id);

		void accept_transaction_to_mempool(const transaction& tx);
		std::vector<transaction> get_tx_mempool() const;
		void generate_block();

		fc::variant get_state() const;
		std::string get_state_json() const;

	private:
		// @throws exception
		std::shared_ptr<generic_evaluator> get_operation_evaluator(transaction* tx, const operation& op);
	};
}
