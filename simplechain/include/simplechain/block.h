#pragma once
#include <simplechain/transaction.h>
#include <vector>

namespace simplechain {
	struct block {
		uint64_t block_number;
		fc::time_point_sec block_time;
		std::vector<transaction> txs;

		hash_t digest() const;
		std::string digest_str() const;

		std::string block_hash() const;
	};
}

FC_REFLECT(simplechain::block, (block_number)(block_time)(txs))