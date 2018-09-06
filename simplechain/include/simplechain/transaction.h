#pragma once
#include <simplechain/config.h>
#include <simplechain/operation.h>
#include <simplechain/operations.h>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/base64.hpp>
#include <fc/time.hpp>
#include <memory>
#include <vector>

namespace simplechain {
	typedef fc::sha256 hash_t;

	struct transaction {
		fc::time_point_sec tx_time;
		std::vector<operation> operations;

		hash_t digest() const;
		std::string digest_str() const;
		std::string tx_hash() const;
	};
}

FC_REFLECT(simplechain::transaction, (tx_time)(operations))
