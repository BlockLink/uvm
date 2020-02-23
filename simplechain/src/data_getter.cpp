#include <data_getter.hpp>
#include <uvm/uvm_api.h>
#include <http_client_sync.hpp>
#include <simplechain/storage.h>
#include <simplechain/contract_entry.h>
#include <fc/crypto/hex.hpp>
#include <boost/algorithm/hex.hpp>

namespace simplechain {
	std::string hxrpchost ;
	std::string hxrpcport ;
	std::string hxrpcpage;

	//std::string hxrpchost = "106.13.107.239";
	//std::string hxrpcport = "19035";
	//std::string hxrpcpage = "api";

	void Data_getter::setPara(const std::string &h, const std::string &p, const std::string &pa) {
		hxrpchost = h;
		hxrpcport = p;
		hxrpcpage = pa;
	}

	std::shared_ptr<UvmContractInfo> Data_getter::get_contract_info(const char *contract_address) {
		std::string res;
		fc::mutable_variant_object postdata;
		postdata["id"] = 0;
		postdata["method"] = "get_simple_contract_info";
		fc::variants arr;
		arr.push_back(std::string(contract_address));
		postdata["params"] = arr;
		auto data = fc::json::to_string(postdata);

		if (HTTP_CLIENT::post(hxrpchost, hxrpcport, hxrpcpage, data, res) != 0) {
			return nullptr;
		}
		else {
			try
			{
				auto v = fc::json::from_string(res);
				//auto code = v["code_printable"];
				uvm::blockchain::Code code;
				fc::from_variant(v["result"]["code_printable"], code);

				UvmContractInfo info;				
				std::copy(code.abi.begin(), code.abi.end(), std::back_inserter(info.contract_apis));
				std::copy(code.offline_abi.begin(), code.offline_abi.end(), std::back_inserter(info.contract_apis));

				for (const auto&item : code.api_arg_types) {
					info.contract_api_arg_types[item.first] = std::vector<UvmTypeInfoEnum>(item.second);
				}
				return std::make_shared<UvmContractInfo>(info);
				
			}
			catch (...) {
				return nullptr;
			}
		}
	}

	std::shared_ptr<uvm::blockchain::Code> Data_getter::get_contract_code(const char *contract_address) {
		std::string res;
		fc::mutable_variant_object postdata;
		postdata["id"] = 0;
		postdata["method"] = "get_contract_info";
		fc::variants arr;
		arr.push_back(std::string(contract_address));
		postdata["params"] = arr;
		auto data = fc::json::to_string(postdata);

		if (HTTP_CLIENT::post(hxrpchost, hxrpcport, hxrpcpage, data, res) != 0) {
			return nullptr;
		}
		else {
			try
			{
				auto v = fc::json::from_string(res);
				//auto code = v["code_printable"];
				uvm::blockchain::Code code;
				fc::from_variant(v["result"]["code_printable"], code);
				return std::make_shared<uvm::blockchain::Code>(code);
			}
			catch (...) {
				return nullptr;
			}
		}
	}

	std::shared_ptr<contract_object> Data_getter::get_contract_object(const char *contract_address) {
		std::string res;
		fc::mutable_variant_object postdata;
		postdata["id"] = 0;
		postdata["method"] = "get_contract_info";
		fc::variants arr;
		//arr.reserve(2);
		arr.push_back(std::string(contract_address));
		postdata["params"] = arr;
		auto data = fc::json::to_string(postdata);

		if (HTTP_CLIENT::post(hxrpchost, hxrpcport, hxrpcpage, data, res) != 0) {
			return nullptr;
		}
		else {
			try
			{
				auto v = fc::json::from_string(res);
				//auto code = v["code_printable"];
				contract_object ob;
				fc::from_variant(v["result"], ob);
				return std::make_shared<contract_object>(ob);
			}
			catch (...) {
				return nullptr;
			}
		}
	}

	UvmStorageValue Data_getter::get_contract_storage(lua_State *L,const char *contract_address, const std::string& key) {
		UvmStorageValue null_storage;
		std::string res;
		fc::mutable_variant_object postdata;
		postdata["id"] = 0;
		postdata["method"] = "get_contract_storage";
		fc::variants arr;
		arr.push_back(std::string(contract_address));
		arr.push_back(key);
		postdata["params"] = arr;
		auto data = fc::json::to_string(postdata);

		//{"id":1,"jsonrpc":"2.0","result":{"id":"1.28.1670179","contract_address":"HXCJb6BiDdSL9Duo9UtXaxncyRHcNsfQB7cV","storage_name":"supply","storage_value":"1b002386f26fc10000"}}
		if (HTTP_CLIENT::post(hxrpchost, hxrpcport, hxrpcpage, data, res) != 0) {
			return null_storage;
		}
		else {
			try
			{
				auto v = fc::json::from_string(res);
				auto hex_string = v["result"]["storage_value"].as_string();
				StorageDataType s;
				//std::vector<char> chars;
				boost::algorithm::unhex(hex_string, std::inserter(s.storage_data, s.storage_data.begin()));
				return StorageDataType::create_lua_storage_from_storage_data(L, s);
			}
			catch (...) {
				return null_storage;
			}
		}
	}
}

