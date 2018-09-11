#pragma once
#include <simplechain/operations_helper.h>
#include <simplechain/blockchain.h>
#include <simplechain/block.h>
#include <simplechain/transaction.h>
#include <string>
#include <vector>
#include <memory>
#include <boost/functional.hpp>
#include <boost/function.hpp>
#include <server_http.hpp>

namespace simplechain {

	using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

	namespace rpc {

		typedef std::vector<fc::variant> RpcRequestParams;

		struct RpcRequest {
			std::string method;
			RpcRequestParams params;
		};

		typedef fc::variant RpcResultType;

		struct RpcResponse {
			bool has_error = false;
			std::string error;
			int32_t error_code = 0;
			RpcResultType result;
		};

		typedef boost::function<RpcResultType(blockchain*, HttpServer*, const RpcRequestParams&)> RpcHandlerType;

		// mint(caller_address: string, asset_id: int, amount: int)
		RpcResultType mint(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// create_contract_from_file(caller_address: string, contract_filepath: string, gas_limit: int, gas_price: int)
		RpcResultType create_contract_from_file(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// create_contract(caller_address: string, contract_base64: string, gas_limit: int, gas_price: int)
		RpcResultType create_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// invoke_contract(caller_address: string, contract_address: string, api_name: string, api_args: [string], gas_limit: int, gas_price: int)
		RpcResultType invoke_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType invoke_contract_offline(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType generate_block(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType exit_chain(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_chain_state(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType list_accounts(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType list_assets(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType list_contracts(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_contract_info(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_account_balances(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_contract_storages(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		RpcResultType get_storage(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// TODO: deposit to contract
	}
}