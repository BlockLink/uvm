#include <simplechain/transaction.h>
#include <fc/io/raw.hpp>

namespace simplechain {
	hash_t transaction::digest() const {
		return hash_t::hash(this);
	}
	std::string transaction::digest_str() const {
		return digest().str();
	}
	std::string transaction::tx_hash() const {
		return digest_str();
	}
}