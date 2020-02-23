#pragma once
#include <uvm/uvm_api.h>
#include <simplechain/contract_object.h>


namespace simplechain {

	class Data_getter {
	public:	
		static void setPara(const std::string &h, const std::string &p, const std::string &pa);
		static std::shared_ptr<uvm::blockchain::Code> get_contract_code(const char *contract_address);
		static std::shared_ptr<contract_object> get_contract_object(const char *contract_address);
		static std::shared_ptr<UvmContractInfo> get_contract_info(const char *contract_address);
		static UvmStorageValue get_contract_storage(lua_State *L,const char *contract_address, const std::string& key);
	};
	
}
