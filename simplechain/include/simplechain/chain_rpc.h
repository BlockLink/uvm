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

		// open_debugger() when call next contract-related rpc, start contract debugger. return debugger-session-id
		RpcResultType open_debugger(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// close_debugger(session_id: string) close contract debugger session by session id
		RpcResultType close_debugger(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// list_debugger_sessions()
		RpcResultType list_debugger_sessions(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// debugger_step_in(session_id: string)
		RpcResultType debugger_step_in(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// debugger_step_out(session_id: string)
		RpcResultType debugger_step_out(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// debugger_continue(session_id: string)
		RpcResultType debugger_continue(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// debugger_add_breakpoint(contract_address: string, source_line: int)
		RpcResultType debugger_add_breakpoint(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// debugger_remove_breakpoint(contract_address: string, source_line: int)
		RpcResultType debugger_remove_breakpoint(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// list_breakpoints()
		RpcResultType list_breakpoints(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// clear_breakpoints()
		RpcResultType clear_breakpoints(blockchain* chain, HttpServer* server, const RpcRequestParams& params);

		// mint(caller_address: string, asset_id: int, amount: int)
		RpcResultType mint(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// create_contract_from_file(caller_address: string, contract_filepath: string, gas_limit: int, gas_price: int)
		RpcResultType create_contract_from_file(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// create_contract(caller_address: string, contract_base64: string, gas_limit: int, gas_price: int)
		RpcResultType create_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// invoke_contract(caller_address: string, contract_address: string, api_name: string, api_args: [string], gas_limit: int, gas_price: int)
		RpcResultType invoke_contract(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// invoke_contract_offline(caller_address: string, contract_address: string, api_name: string, api_args: [string])
		RpcResultType invoke_contract_offline(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// generate_block(count: int)
		RpcResultType generate_block(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// exit_chain()
		RpcResultType exit_chain(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// get_chain_state()
		RpcResultType get_chain_state(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// list_accounts()
		RpcResultType list_accounts(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// list_assets()
		RpcResultType list_assets(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// list_contracts()
		RpcResultType list_contracts(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// get_contract_info(contract_address: string)
		RpcResultType get_contract_info(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// get_account_balances(account_address: string)
		RpcResultType get_account_balances(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// get_contract_storages(contract_address: string)
		RpcResultType get_contract_storages(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// get_storage(contract_address: string, storage_key: string)
		RpcResultType get_storage(blockchain* chain, HttpServer* server, const RpcRequestParams& params);
		// TODO: deposit to contract
	}
}