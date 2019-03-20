#include <native_contract/native_exchange_contract.h>
#include <boost/algorithm/string.hpp>
#include <jsondiff/jsondiff.h>
#include <cbor_diff/cbor_diff.h>
#include <uvm/uvm_lutil.h>
#include <uvm/uvm_api.h>
#include <fc/crypto/hex.hpp>

#include <fc/crypto/sha256.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/array.hpp>
#include <fc/crypto/ripemd160.hpp>


namespace fc {
	/*void to_variant(const simplechain::asset& var, variant& vo) {
	fc::mutable_variant_object obj("asset_id", var.asset_id);
	obj("symbol", var.symbol);
	obj("precision", var.precision);
	vo = std::move(obj);
	}*/
	void from_variant(const variant& var, uvm::contract::exchange::OrderInfo& vo) {
		auto obj = var.get_object();
		from_variant(obj["purchaseAsset"], vo.purchaseAsset);
		from_variant(obj["purchaseNum"], vo.purchaseNum);
		from_variant(obj["payAsset"], vo.payAsset);
		from_variant(obj["payNum"], vo.payNum);
		from_variant(obj["nonce"], vo.nonce);
		from_variant(obj["relayer"], vo.relayer);
		from_variant(obj["fee"], vo.fee);
	}
	void from_variant(const variant& var, uvm::contract::exchange::Order& vo) {
		auto obj = var.get_object();
		from_variant(obj["sig"], vo.sig);
		from_variant(obj["id"], vo.id);
		from_variant(obj["orderInfo"], vo.orderInfo);
	}
	void from_variant(const variant& var, uvm::contract::exchange::FillOrder& vo) {
		auto obj = var.get_object();
		from_variant(obj["order"], vo.order);
		from_variant(obj["fillNum"], vo.fillNum);
	}
	void from_variant(const variant& var, uvm::contract::exchange::MatchInfo& vo) {
		auto obj = var.get_object();
		from_variant(obj["takerOrder"], vo.takerOrder);
		from_variant(obj["takerPayNum"], vo.takerPayNum);
		from_variant(obj["fillOrders"], vo.fillOrders);
	}
}

namespace uvm {
	namespace contract {
		using namespace cbor_diff;
		using namespace cbor;
		using namespace uvm::contract;

		

		//  contract
		std::string exchange_native_contract::contract_key() const
		{
			return exchange_native_contract::native_contract_key();
		}

		std::set<std::string> exchange_native_contract::apis() const {
			return { "init", "init_config", "fillOrder","test" };
		}
		std::set<std::string> exchange_native_contract::offline_apis() const {
			return { "name", "state", "feeReceiver"};
		}
		std::set<std::string> exchange_native_contract::events() const {
			return { "Inited", "OrderCanceled", "OrderFilled" };
		}

		static const std::string not_inited_state_of_exchange_contract = "NOT_INITED";
		static const std::string common_state_of_exchange_contract = "COMMON";

		void exchange_native_contract::init_api(const std::string& api_name, const std::string& api_arg)
		{/*
			set_current_contract_storage("name", CborObject::from_string(""));
			set_current_contract_storage("symbol", CborObject::from_string(""));
			set_current_contract_storage("supply", CborObject::from_int(0));
			set_current_contract_storage("precision", CborObject::from_int(0));
			// fast map storages: users, allowed
			set_current_contract_storage("state", CborObject::from_string(not_inited_state_of_exchange_contract));
			auto caller_addr = caller_address_string();
			FC_ASSERT(!caller_addr.empty(), "caller_address can't be empty");
			set_current_contract_storage("admin", CborObject::from_string(caller_addr));
			return;
			*/

			set_current_contract_storage("name", CborObject::from_string(""));
			set_current_contract_storage("feeReceiver", CborObject::from_string(""));
			
			// fast map storages: canceledOrders, filledOrders
			set_current_contract_storage("state", CborObject::from_string(not_inited_state_of_exchange_contract));
			auto caller_addr = caller_address_string();
			FC_ASSERT(!caller_addr.empty(), "caller_address can't be empty");
			set_current_contract_storage("admin", CborObject::from_string(caller_addr));
			return;
		}

		std::string exchange_native_contract::check_admin()
		{
			const auto& caller_addr = caller_address_string();
			const auto& admin = get_string_current_contract_storage("admin");
			if (admin == caller_addr)
				return admin;
			throw_error("only admin can call this api");
			return "";
		}

		std::string exchange_native_contract::get_storage_state()
		{
			const auto& state = get_string_current_contract_storage("state");
			return state;
		}

		std::string exchange_native_contract::get_from_address()
		{
			return caller_address_string(); // FIXME: when get from_address, caller maybe other contract
		}

		static bool is_numeric(std::string number)
		{
			char* end = 0;
			std::strtod(number.c_str(), &end);

			return end != 0 && *end == 0;
		}

		using uvm::lua::api::global_uvm_chain_api;

		std::string getNormalAddress(const fc::ecc::public_key& pub) {
			if (!global_uvm_chain_api) {
				return "invalid global_uvm_chain_api";
			}
			else {
				return global_uvm_chain_api->pubkey_to_address_string(pub);
			}
		}

		//order: {orderInfo:{purchaseAsset:"HC",purchaseNum:20,payAsset:"HX",payNum:100,nounce:"13df",relayer:"HXsrsfsfe3",fee:0.0001},sig:"rsowor233",id:"gsewwsg"}
		static std::string checkOrder(CborObjectP orderp, int takerpayNum,std::string addr) {
			auto order = orderp->as_map();
			auto orderInfo = order.at("orderInfo");
			auto sig_hex = order.at("sig")->as_string();
			
			cbor::output_dynamic output;
			cbor::encoder encoder(output);
			encoder.write_cbor_object(orderInfo.get());
			const auto& output_hex = output.hex();
			 fc::sha256 orderinfoDigest(output_hex);

			std::vector<char> sig_bytes(sig_hex.size() / 2);
			auto bytes_count = fc::from_hex(sig_hex, sig_bytes.data(), sig_bytes.size());
			if (bytes_count != sig_bytes.size()) {
				return "parse sig hex to bytes error";
			}

			fc::ecc::compact_signature compact_sig;
			if (sig_bytes.size() > compact_sig.size())
				return "invalid sig bytes size";
			memcpy(compact_sig.data, sig_bytes.data(), sig_bytes.size());

			auto recoved_public_key = fc::ecc::public_key(compact_sig, orderinfoDigest, false);
			if (!recoved_public_key.valid()) {
				return "invalid signature";
			}
			auto pubk_base58 = recoved_public_key.to_base58();
			printf("recoverpubk_base58:%s", pubk_base58.c_str());

			auto orderOwner = getNormalAddress(recoved_public_key);
			if (orderOwner != addr) {
				return "orderOwner not match";
			}

			///////////////////////////////////
			auto orderInfoMap = orderInfo->as_map();

			auto purchaseAsset = orderInfoMap["purchaseAsset"]->as_string();
			auto purchaseNum = orderInfoMap["purchaseNum"]->as_int();
			auto payAsset = orderInfoMap["payAsset"]->as_string();
			auto payNum = orderInfoMap["payNum"]->as_int();
			auto relayer = orderInfoMap["relayer"]->as_string();
			auto fee = orderInfoMap["fee"]->as_float64();

			

			 ///////////////////////////////////////////////
			const auto& public_key_chars = recoved_public_key.serialize();
			std::vector<unsigned char> public_key_bytes(public_key_chars.begin(), public_key_chars.end());

			std::vector<char> chars(public_key_bytes.size());
			memcpy(chars.data(), public_key_bytes.data(), public_key_bytes.size());
			auto public_key_hex = fc::to_hex(chars);
			printf("public_key_hex:%s", public_key_hex.c_str());
			return "OK";
		}

		//order: {orderInfo:{purchaseAsset:"HC",purchaseNum:20,payAsset:"HX",payNum:100,nounce:"13df",relayer:"HXsrsfsfe3",fee:0.0001},sig:"rsowor233",id:"gsewwsg"}
		static std::string makeOrder(std::string addr) {
			CborMapValue order;
			CborMapValue orderInfo;
			orderInfo["purchaseAsset"] = CborObject::from_string("HC");
			orderInfo["purchaseNum"] = CborObject::from_int(20);
			orderInfo["payAsset"] = CborObject::from_string("HX");
			orderInfo["payNum"] = CborObject::from_int(80);
			orderInfo["nonce"] = CborObject::from_string("WWW");

			auto orderInfoP = CborObject::create_map(orderInfo);
			
			cbor::output_dynamic output;
			cbor::encoder encoder(output);
			encoder.write_cbor_object(orderInfoP.get());
			const auto& output_hex = output.hex();
			fc::sha256 orderinfoDigest(output_hex);

			auto prik = fc::ecc::private_key::generate();
			auto pubk = prik.get_public_key();
			auto pubk_base58 = pubk.to_base58();
			printf("pubk_base58:%s\n", pubk_base58.c_str());

			auto orderOwner = getNormalAddress(pubk);

			auto sig = prik.sign_compact(orderinfoDigest);
						
			std::vector<char> chars(sig.size());
			memcpy(chars.data(), sig.data, sig.size());
			auto sig_hex = fc::to_hex(chars);

			order["orderInfo"] = orderInfoP;
			order["sig"] = CborObject::from_string(sig_hex);
			order["id"] =  CborObject::from_string("ssssiiiiiiii");

			auto orderP = CborObject::create_map(order);

			auto r = checkOrder(orderP, 20, orderOwner);
			printf("r:%s", r.c_str());
			return "OK";
		}


		static bool is_integral(std::string number)
		{
			return is_numeric(number.c_str()) && std::strchr(number.c_str(), '.') == 0;
		}

		// arg format: feeReceiver,percentage
		void exchange_native_contract::init_config_api(const std::string& api_name, const std::string& api_arg)
		{
			check_admin();
			if (get_storage_state() != not_inited_state_of_exchange_contract)
				throw_error("this exchange contract inited before");
			std::vector<std::string> parsed_args;
			boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
			if (parsed_args.size() != 2)
				throw_error("argument format error, need format: feeReceiver,percentage");
			std::string feeReceiver = parsed_args[0];
			boost::trim(feeReceiver);
			std::string percentage = parsed_args[1];
			boost::trim(percentage);
			if (feeReceiver.empty() || percentage.empty())
				throw_error("argument format error, need format: feeReceiver,percentage");
			if (!is_numeric(percentage))
				throw_error("argument format error, percentage is not numeric");

			set_current_contract_storage("feeReceiver", CborObject::from_string(feeReceiver));
			set_current_contract_storage("percentage", CborObject::from_string(percentage));
			set_current_contract_storage("state", CborObject::from_string(common_state_of_exchange_contract));
			emit_event("Inited", feeReceiver);
			return;
		}


		void exchange_native_contract::state_api(const std::string& api_name, const std::string& api_arg)
		{
			const auto& state = get_storage_state();
			set_api_result(state);
			return;
		}

		cbor::CborObjectP order_variant_to_cbor(fc::mutable_variant_object v) {
			CborMapValue order;
			CborMapValue orderInfo;
			
			if (v.find("orderInfo") == v.end() || (!v["orderInfo"].is_object())) {
				return nullptr;
			}
			auto infov = v["orderInfo"].as<fc::mutable_variant_object>();
			

			orderInfo["purchaseAsset"] = CborObject::from_string(infov["purchaseAsset"].as_string());
			orderInfo["purchaseNum"] = CborObject::from_int(infov["purchaseNum"].as_int64());
			orderInfo["payAsset"] = CborObject::from_string(infov["payAsset"].as_string());
			orderInfo["payNum"] = CborObject::from_int(infov["payNum"].as_int64());
			orderInfo["nonce"] = CborObject::from_string(infov["nonce"].as_string());
			orderInfo["relayer"] = CborObject::from_string(infov["relayer"].as_string());
			orderInfo["fee"] = CborObject::from_string(infov["fee"].as_string());

			auto orderInfoP = CborObject::create_map(orderInfo);
			order["orderInfo"] = orderInfoP;
			order["sig"] = CborObject::from_string(v["sig"].as_string());
			order["id"] = CborObject::from_string(v["id"].as_string());

			auto orderP = CborObject::create_map(order);
			return orderP;
		}

		cbor::CborObjectP order_to_cbor(exchange::Order v) {
			CborMapValue order;
			CborMapValue orderInfo;

			orderInfo["purchaseAsset"] = CborObject::from_string(v.orderInfo.purchaseAsset);
			orderInfo["purchaseNum"] = CborObject::from_int(v.orderInfo.purchaseNum);
			orderInfo["payAsset"] = CborObject::from_string(v.orderInfo.payAsset);
			orderInfo["payNum"] = CborObject::from_int(v.orderInfo.payNum);
			orderInfo["nonce"] = CborObject::from_string(v.orderInfo.nonce);
			orderInfo["relayer"] = CborObject::from_string(v.orderInfo.relayer);
			orderInfo["fee"] = CborObject::from_string(v.orderInfo.fee);

			auto orderInfoP = CborObject::create_map(orderInfo);
			order["orderInfo"] = orderInfoP;
			order["sig"] = CborObject::from_string(v.sig);
			order["id"] = CborObject::from_string(v.id);

			auto orderP = CborObject::create_map(order);
			return orderP;
		}

		//order: {orderInfo:{purchaseAsset:"HC",purchaseNum:20,payAsset:"HX",payNum:100,nounce:"13df",relayer:"HXsrsfsfe3",fee:0.0001},sig:"rsowor233"}
		//{takerOrder:order,takerPayNum:80,fillOrders:[{order:order1,fillNum:30},{order:order2,fillNum:50}]
		void exchange_native_contract::fillOrder_api(const std::string& api_name, const std::string& api_args_utf8)
		{
			//if (get_storage_state() != common_state_of_exchange_contract)
			//	throw_error("this exchange contract state is not common");

			auto args = fc::json::from_string(api_args_utf8);
			if (!args.is_object()) {
				throw_error("args not map");
			}
			exchange::MatchInfo matchinfo;
			fc::from_variant(args, matchinfo);

			//args_map.

			return;
		}

		
		
		void exchange_native_contract::test_api(const std::string& api_name, const std::string& api_arg_hexstr)
		{
			auto addr = caller_address_string();
			makeOrder(addr);
			return;
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////


		void exchange_native_contract::invoke(const std::string& api_name, const std::string& api_arg) {
			std::map<std::string, std::function<void(const std::string&, const std::string&)>> apis = {
				{ "init", std::bind(&exchange_native_contract::init_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "init_config", std::bind(&exchange_native_contract::init_config_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "fillOrder", std::bind(&exchange_native_contract::fillOrder_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "test", std::bind(&exchange_native_contract::test_api, this, std::placeholders::_1, std::placeholders::_2) },
			
			{ "state", std::bind(&exchange_native_contract::state_api, this, std::placeholders::_1, std::placeholders::_2) },

			};
			if (apis.find(api_name) != apis.end())
			{
				apis[api_name](api_name, api_arg);
				set_invoke_result_caller();
				add_gas(gas_count_for_api_invoke(api_name));
				return;
			}
			throw_error("token api not found");
		}

	}
}


