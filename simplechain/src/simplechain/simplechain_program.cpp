#include <simplechain/simplechain.h>
#include <iostream>

using namespace simplechain;

int main(int argc, char** argv) {
	std::cout << "Hello, simplechain based on uvm" << std::endl;
	// TODO: catch exception and print error
	auto chain = std::make_shared<simplechain::blockchain>();

	std::string contract1_addr;
	std::string caller_addr = "caller1";
	{
		auto tx1 = std::make_shared<transaction>();
		auto op1 = std::make_shared<contract_create_operation>();
		op1->caller_address = caller_addr;
		// load gpc file
		std::string contract1_gpc_filepath("../test/test_contracts/token.gpc"); // TODO: load from command line arguments
		auto contract1 = ContractHelper::load_contract_from_file(contract1_gpc_filepath);
		op1->contract_code = contract1;
		op1->gas_limit = 1000000;
		op1->gas_price = 10;
		op1->op_time = fc::time_point_sec(fc::time_point::now());
		tx1->operations.push_back(*op1);
		tx1->tx_time = fc::time_point_sec(fc::time_point::now());
		contract1_addr = op1->calculate_contract_id();
		chain->apply_transaction(tx1);
	}
	{
		auto tx = std::make_shared<transaction>();
		contract_invoke_operation op;
		op.caller_address = caller_addr;
		op.contract_address = contract1_addr;
		op.contract_api = "init_token";
		op.contract_args.push_back("test,TEST,10000,100");
		op.gas_limit = 100000;
		op.gas_price = 10;
		op.op_time = fc::time_point_sec(fc::time_point::now());
		tx->operations.push_back(op);
		tx->tx_time = fc::time_point_sec(fc::time_point::now());
		chain->apply_transaction(tx);
	}
	return 0;
}