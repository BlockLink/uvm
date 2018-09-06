#include <simplechain/block.h>

namespace simplechain {
	hash_t block::digest() const {
		return hash_t::hash(this);
	}
	std::string block::digest_str() const {
		return digest().str();
	}
	std::string block::block_hash() const {
		return digest_str();
	}
}
