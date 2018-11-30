#include <simplechain/transaction.h>
#include <fc/io/raw.hpp>
#include <simplechain/operations.h>

namespace simplechain {
	hash_t transaction::digest() const {
		return hash_t::hash(*this);
	}
	std::string transaction::digest_str() const {
		return digest().str();
	}
	std::string transaction::tx_hash() const {
		return digest_str();
	}
	fc::mutable_variant_object transaction::to_json() const {
		fc::mutable_variant_object info;
		info["hash"] = tx_hash();
		info["id"] = info["hash"];
		info["tx_time"] = tx_time.sec_since_epoch();
		fc::variants operations_json;
		for (const auto& op : operations) {
			fc::mutable_variant_object op_json;
			switch (op.which()) {
			case operation::tag<mint_operation>::value: {
				op_json = op.get<mint_operation>().to_json();
			} break;
			case operation::tag<transfer_operation>::value: {
				op_json = op.get<transfer_operation>().to_json();
			} break;
			case operation::tag<contract_create_operation>::value: {
				op_json = op.get<contract_create_operation>().to_json();
			} break;
			case operation::tag<contract_invoke_operation>::value: {
				op_json = op.get<contract_invoke_operation>().to_json();
			} break;
			default: {
				auto err = std::string("unknown operation type ") + std::to_string(op.which()) + " in transaction::to_json()";
				throw uvm::core::UvmException(err.c_str());
			}
			}
			operations_json.push_back(op_json);
		}
		info["operations"] = operations_json;
		return info;
	}
}