#include <simplechain/chain_rpc.h>
#include <simplechain/contract_helper.h>
#include <simplechain/operations.h>
#include <simplechain/transaction.h>
#include <simplechain/block.h>
#include <simplechain/blockchain.h>
#include <streambuf>
#include <istream>
#include <ostream>
#include <boost/asio.hpp>

namespace simplechain {
	namespace rpc {

		RpcResultType mint(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			auto caller_addr = params.at(0).as_string();
			auto asset_id = params.at(1).as_uint64();
			auto amount = params.at(2).as_int64();
			auto tx = std::make_shared<transaction>();
			auto op = operations_helper::mint(caller_addr, asset_id, amount);
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
			auto api_args_json = params.at(3).as<fc::variants>();
			std::vector<std::string> api_args;
			for (const auto& api_arg_json : api_args_json) {
				api_args.push_back(api_arg_json.as_string());
			}

			auto gas_limit = params.at(4).as_uint64();
			auto gas_price = params.at(5).as_uint64();

			auto tx = std::make_shared<transaction>();
			auto op = operations_helper::invoke_contract(caller_addr, contract_address, api_name, api_args);
			tx->operations.push_back(op);
			tx->tx_time = fc::time_point_sec(fc::time_point::now());

			chain->evaluate_transaction(tx);
			chain->accept_transaction_to_mempool(*tx);
			return tx->tx_hash();
		}

		RpcResultType generate_block(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			int count = 1;
			if (!params.empty()) {
				count = params.at(0).as_int64();
				FC_ASSERT(count > 0);
			}
			fc::variants block_ids;
			for (int i = 0; i < count; i++) {
				chain->generate_block();
				block_ids.push_back(chain->latest_block().block_hash());
			}
			return block_ids;
		}

		RpcResultType exit_chain(blockchain* chain, HttpServer* server, const RpcRequestParams& params) {
			server->stop();
			return true;
		}

	}
}
