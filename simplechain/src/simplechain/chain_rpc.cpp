#include <simplechain/chain_rpc.h>
#include <simplechain/contract_helper.h>
#include <simplechain/operations.h>
#include <simplechain/transaction.h>
#include <simplechain/block.h>
#include <simplechain/blockchain.h>
#include <simplechain/contract.h>
#include <uvm/uvm_lib.h>
#include <streambuf>
#include <istream>
#include <ostream>
#include <boost/asio.hpp>
#include <fc/io/json.hpp>
#include <cbor_diff/cbor_diff.h>
#include <fc/crypto/hex.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/ripemd160.hpp>


namespace simplechain {
	namespace rpc {

		static void params_assert(bool cond, const std::string& msg = "") {
			if (!cond) {
				throw uvm::core::UvmException(msg.empty() ? "params invalid" : msg);
			}
		}

		RpcResultType mint(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& caller_addr = params.at(0).as_string();
			auto asset_id = (asset_id_t) params.at(1).as_uint64();
			auto amount = params.at(2).as_int64();
			auto tx = std::make_shared<transaction>();
			auto op = operations_helper::mint(caller_addr, asset_id, amount);
			tx->tx_time = fc::time_point_sec(fc::time_point::now());
			tx->operations.push_back(op);

			chain->evaluate_transaction(tx);
			chain->accept_transaction_to_mempool(*tx);
			
			return tx->tx_hash();
		}

		RpcResultType transfer(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& from_addr = params.at(0).as_string();
			const auto& to_addr = params.at(1).as_string();
			auto asset_id = (asset_id_t) params.at(2).as_uint64();
			auto amount = (share_type) params.at(3).as_int64();
			auto tx = std::make_shared<transaction>();
			auto op = operations_helper::transfer(from_addr, to_addr, asset_id, amount);
			tx->tx_time = fc::time_point_sec(fc::time_point::now());
			tx->operations.push_back(op);

			chain->evaluate_transaction(tx);
			chain->accept_transaction_to_mempool(*tx);

			return tx->tx_hash();
		}

		RpcResultType create_contract_from_file(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			auto caller_addr = params.at(0).as_string();
			auto contract_filepath = params.at(1).as_string();
			auto gas_limit = params.at(2) .as_uint64();
			auto gas_price = params.at(3).as_uint64();
			auto tx = std::make_shared<transaction>();
			auto op = operations_helper::create_contract_from_file(caller_addr, contract_filepath, gas_limit, gas_price);
			tx->operations.push_back(op);
			tx->tx_time = fc::time_point_sec(fc::time_point::now());
			const auto& contract_addr = op.calculate_contract_id();

			chain->evaluate_transaction(tx);
			chain->accept_transaction_to_mempool(*tx);

			fc::mutable_variant_object res;
			res["contract_address"] = contract_addr;
			res["txid"] = tx->tx_hash();
			return res;
		}
		
		RpcResultType create_native_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			auto caller_addr = params.at(0).as_string();
			auto contract_type = params.at(1).as_string();
			auto gas_limit = params.at(2).as_uint64();
			auto gas_price = params.at(3).as_uint64();
			auto tx = std::make_shared<transaction>();
			auto op = operations_helper::create_native_contract(caller_addr, contract_type);
			tx->operations.push_back(op);
			tx->tx_time = fc::time_point_sec(fc::time_point::now());
			const auto& contract_addr = op.calculate_contract_id();

			chain->evaluate_transaction(tx);
			chain->accept_transaction_to_mempool(*tx);

			fc::mutable_variant_object res;
			res["contract_address"] = contract_addr;
			res["txid"] = tx->tx_hash();
			return res;
		}
		RpcResultType create_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			auto caller_addr = params.at(0).as_string();
			auto contract_base64 = params.at(1).as_string();
			auto gas_limit = params.at(2).as_uint64();
			auto gas_price = params.at(3).as_uint64();

			auto tx = std::make_shared<transaction>();
			
			const auto& contract_str = fc::base64_decode(contract_base64);
			std::vector<char> contract_buf(contract_str.size());
			memcpy(contract_buf.data(), contract_str.data(), contract_str.size());

			boost::asio::streambuf contract_stream;
			std::ostream os(&contract_stream);
			os.write(contract_buf.data(), contract_buf.size());
			std::istream contract_istream(&contract_stream);
			StreamFileWrapper sfw(&contract_istream);
			const auto& code = ContractHelper::load_contract_from_common_stream_and_close(&sfw, contract_buf.size());

			auto op = operations_helper::create_contract(caller_addr, code, gas_limit, gas_price);
			tx->operations.push_back(op);
			tx->tx_time = fc::time_point_sec(fc::time_point::now());
			const auto& contract_addr = op.calculate_contract_id();

			chain->evaluate_transaction(tx);
			chain->accept_transaction_to_mempool(*tx);

			fc::mutable_variant_object res;
			res["contract_address"] = contract_addr;
			res["txid"] = tx->tx_hash();
			return res;
		}

		RpcResultType invoke_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			auto caller_addr = params.at(0).as_string();
			auto contract_address = params.at(1).as_string();
			auto api_name = params.at(2).as_string();
			params_assert(params.at(3).is_array());
			auto api_args_json = params.at(3).as<fc::variants>();
			std::vector<std::string> api_args;
			for (const auto& api_arg_json : api_args_json) {
				api_args.push_back(api_arg_json.as_string());
			}
			auto deposit_asset_id = params.at(4).as_uint64();
			auto deposit_amount = params.at(5).as_uint64();
			auto gas_limit = params.at(6).as_uint64();
			auto gas_price = params.at(7).as_uint64();

			auto tx = std::make_shared<transaction>();
			auto op = operations_helper::invoke_contract(caller_addr, contract_address, api_name, api_args, gas_limit, gas_price);
			op.deposit_amount = deposit_amount;
			op.deposit_asset_id = (asset_id_t) deposit_asset_id;
			tx->operations.push_back(op);
			tx->tx_time = fc::time_point_sec(fc::time_point::now());

			auto op_result = chain->evaluate_transaction(tx);
			fc::mutable_variant_object res;
			res["txid"] = tx->tx_hash();
			if (op_result) {
				contract_invoke_result* contract_result = (contract_invoke_result*)op_result.get();
				res["api_result"] = contract_result->api_result;
				res["exec_succeed"] = contract_result->exec_succeed;
				res["gas_used"] = contract_result->gas_used;
				if (!contract_result->exec_succeed)
					res["error"] = contract_result->error;
			}
			chain->accept_transaction_to_mempool(*tx);
			return res;
		}

		RpcResultType invoke_contract_offline(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			auto caller_addr = params.at(0).as_string();
			auto contract_address = params.at(1).as_string();
			auto api_name = params.at(2).as_string();
			params_assert(params.at(3).is_array());
			auto api_args_json = params.at(3).as<fc::variants>();
			auto deposit_asset_id = (asset_id_t) params.at(4).as_uint64();
			auto deposit_amount = params.at(5).as_uint64();

			std::vector<std::string> api_args;
			for (const auto& api_arg_json : api_args_json) {
				api_args.push_back(api_arg_json.as_string());
			}

			auto tx = std::make_shared<transaction>();
			auto op = operations_helper::invoke_contract(caller_addr, contract_address, api_name, api_args, 10000000);
			op.deposit_amount = deposit_amount;
			op.deposit_asset_id = (asset_id_t) deposit_asset_id;
			tx->operations.push_back(op);
			tx->tx_time = fc::time_point_sec(fc::time_point::now());

			auto op_result = chain->evaluate_transaction(tx);
			fc::mutable_variant_object res;
			res["txid"] = tx->tx_hash();
			if (op_result) {
				contract_invoke_result* contract_result = (contract_invoke_result*) op_result.get();
				res["api_result"] = contract_result->api_result;
				res["exec_succeed"] = contract_result->exec_succeed;
				if (!contract_result->exec_succeed)
					res["error"] = contract_result->error;
			}
			return res;
		}

		RpcResultType generate_block(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			int count = 1;
			if (!params.empty()) {
				count = (int) params.at(0).as_int64();
				if (count <= 0) {
					throw uvm::core::UvmException("need generate at least 1 block");
				}
			}
			fc::variants block_ids;
			for (int i = 0; i < count; i++) {
				chain->generate_block();
				block_ids.push_back(chain->head_block_hash());
			}
			return block_ids;
		}

		RpcResultType get_block_by_height(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& block_height = params.at(0).as_uint64();
			auto block = chain->get_block_by_number(block_height);
			if (block) {
				return block->to_json();
			}
			else {
				return nullptr;
			}
		}

		RpcResultType get_tx(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& tx_id = params.at(0).as_string();
			auto tx = chain->get_trx_by_hash(tx_id);
			if (tx) {
				return tx->to_json();
			}
			else {
				return nullptr;
			}
		}

		RpcResultType get_tx_receipt(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& tx_id = params.at(0).as_string();
			const auto& receipt = chain->get_tx_receipt(tx_id);
			if (receipt) {
				return receipt->to_json();
			}
			else {
				return nullptr;
			}
		}

		RpcResultType exit_chain(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			server->stop();
			return true;
		}

		RpcResultType get_chain_state(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& state = chain->get_state();
			return state;
		}
		
		RpcResultType list_accounts(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& accounts = chain->get_account_addresses();
			fc::variant account_addresses;
			fc::to_variant(accounts, account_addresses);
			return account_addresses;
		}
		RpcResultType list_assets(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& assets = chain->get_assets();
			fc::variant assets_obj;
			fc::to_variant(assets, assets_obj);
			return assets_obj;
		}
		RpcResultType list_contracts(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& contracts = chain->get_contracts();
			fc::variants contract_ids;
			for (const auto& contract : contracts) {
				contract_ids.push_back(contract.contract_address);
			}
			return contract_ids;
		}
		RpcResultType get_contract_info(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& contract_address = params.at(0).as_string();
			auto contract = chain->get_contract_by_address(contract_address);
			if (!contract) {
				throw uvm::core::UvmException(std::string("Can't find contract ") + contract_address);
			}
			fc::variant obj;
			fc::to_variant(*contract, obj);
			return obj;
		}
		RpcResultType get_account_balances(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& addr = params.at(0).as_string();
			const auto& balances = chain->get_account_balances(addr);
			fc::variant res;
			fc::to_variant(balances, res);
			return res;
		}
		RpcResultType get_contract_storages(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& addr = params.at(0).as_string();
			const auto& storages = chain->get_contract_storages(addr);
			fc::mutable_variant_object storages_json;
			for (const auto& p : storages) {
				storages_json[p.first] = fc::json::from_string(p.second.as<std::string>());
			}
			return storages_json;
		}
		RpcResultType get_storage(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			const auto& addr = params.at(0).as_string();
			const auto& storage_name = params.at(1).as_string();
			const auto& storage = chain->get_storage(addr, storage_name);
			auto storage_value = cbor_diff::cbor_decode(storage.storage_data);
			auto scope = std::make_shared < uvm::lua::lib::UvmStateScope>();
			auto uvm_storage_data = cbor_to_uvm_storage_value(scope->L(), storage_value.get());
			auto storage_json = simplechain::uvm_storage_value_to_json(uvm_storage_data);
			auto res = storage_json;
			return res;
		}

		RpcResultType add_asset(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			asset a;
			a.symbol = params.at(0).as_string();
			a.precision = (asset_id_t)params.at(1).as_uint64();
			fc::mutable_variant_object res;

			if (chain->get_asset_by_symbol(a.symbol) != nullptr) {
				res["result"] = false;
				return res;
			}
			chain->add_asset(a);
			res["result"] = true;
			return res;
		}

		using uvm::lua::api::global_uvm_chain_api;
		RpcResultType generate_key(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			fc::mutable_variant_object res;
			try {
				auto prik = fc::ecc::private_key::generate();
				auto pubk = prik.get_public_key();
				auto pubk_base58 = pubk.to_base58();
				
				auto address_str = global_uvm_chain_api->pubkey_to_address_string(pubk);

				auto private_key_str = prik.get_secret().str();
				auto ss = pubk.serialize();
				auto hex_pubkey = fc::to_hex(ss.data, ss.size());
				
				res["prik"] = private_key_str;
				res["public_key_base58"] = pubk_base58;
				res["hex_pubkey"] = hex_pubkey;
				res["addr"] = address_str;

			}
			catch (fc::exception e) {
				res["exec_succeed"] = false;
				return res;
			}
			return res;
		}


		RpcResultType sign_info(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			fc::mutable_variant_object res;
			try{
				const auto& private_key_str = params.at(0).as_string();
				const auto& infostr = params.at(1).as_string();
				/*std::string infostr;
				if (fc::json::is_valid(info)) {
					auto v = fc::json::from_string(info); //map items order 
					infostr = fc::json::to_string(v);  
				}
				else {
					infostr = info;
				}*/
				
				auto private_key = fc::ecc::private_key::regenerate(fc::sha256(private_key_str));

				auto orderinfoDigest = fc::sha256::hash(infostr);
				auto sig = private_key.sign_compact(orderinfoDigest);

				std::vector<char> chars(sig.size());
				memcpy(chars.data(), sig.data, sig.size());
				auto sig_hex = fc::to_hex(chars);

				auto temp = infostr + sig_hex;
				auto orderID = fc::sha256::hash(temp);
				//fc::sha256 orderID(temp);
				auto id = orderID.str();
			
				res["digest_hex"] = orderinfoDigest.str();
				res["sig_hex"] = sig_hex;
				res["id"] = id;
				res["exec_succeed"] = true;
			}
			catch (fc::exception e) {
				res["exec_succeed"] = false;
				return res;

			}
			return res;
		}

	}
}
