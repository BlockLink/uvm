#include <simplechain/address_helper.h>

namespace simplechain {
	namespace helper {
		bool is_valid_address(const std::string& addr) {
			return !addr.empty();
		}
		bool is_valid_contract_address(const std::string& addr) {
			return addr.find(SIMPLECHAIN_CONTRACT_ADDRESS_PREFIX) == 0 && addr.size() > strlen(SIMPLECHAIN_CONTRACT_ADDRESS_PREFIX);
		}
	}
}