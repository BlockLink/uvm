#include <simplechain/contract.h>
#include <simplechain/contract_entry.h>
#include <fc/crypto/sha512.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/io/raw.hpp>

namespace simplechain {
	contract_create_operation::contract_create_operation()
	: type(operation_type_enum::CONTRACT_CREATE),
	  gas_price(0),
	  gas_limit(0) {}

	contract_create_operation::~contract_create_operation() {

	}
	std::string contract_create_operation::calculate_contract_id() const
	{
		std::string id;
		fc::sha256::encoder enc;
		FC_ASSERT((contract_code != uvm::blockchain::Code()));
		if (contract_code != uvm::blockchain::Code())
		{
			std::pair<uvm::blockchain::Code, fc::time_point_sec> info_to_digest(contract_code, op_time);
			//fc::raw::pack(enc, info_to_digest);
			int32_t a = 123;
			auto sss = fc::raw::pack(a);
			fc::raw::pack(enc, contract_code);
			fc::raw::pack(enc, op_time);
		}

		id = fc::ripemd160::hash(enc.result()).str();
		return id;
	}

	contract_invoke_operation::contract_invoke_operation()
	: type(operation_type_enum::CONTRACT_INVOKE),
	  gas_price(0),
	  gas_limit(0) {}

	contract_invoke_operation::~contract_invoke_operation() {

	}


	void contract_invoke_result::reset()
	{
		api_result.clear();
		storage_changes.clear();
		contract_withdraw.clear();
		contract_balances.clear();
		deposit_to_address.clear();
		deposit_contract.clear();
		transfer_fees.clear();
		events.clear();
		exec_succeed = true;
	}

	void contract_invoke_result::set_failed()
	{
		api_result.clear();
		storage_changes.clear();
		contract_withdraw.clear();
		contract_balances.clear();
		deposit_to_address.clear();
		deposit_contract.clear();
		transfer_fees.clear();
		events.clear();
		exec_succeed = false;
	}

}
