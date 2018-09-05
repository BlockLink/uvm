#pragma once
#include <simplechain/config.h>
#include <simplechain/blockchain.h>
#include <simplechain/transaction.h>
#include <simplechain/contract.h>
#include <simplechain/operation.h>
#include <simplechain/contract_helper.h>
#include <simplechain/operations_helper.h>
#include <server_http.hpp>

namespace simplechain {
	using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

	class RpcServer final {
	private:
		int _port;
	public:
		RpcServer(int port = 8080);
		~RpcServer();

		void start();
	};
}