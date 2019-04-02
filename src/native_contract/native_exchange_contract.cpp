#include <native_contract/native_exchange_contract.h>
#include <boost/algorithm/string.hpp>
#include <jsondiff/jsondiff.h>
#include <cbor_diff/cbor_diff.h>
#include <uvm/uvm_lutil.h>
#include <uvm/uvm_lib.h>
#include <fc/crypto/hex.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/array.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <safenumber/safenumber.h>



namespace fc {
	void from_variant(const variant& var, uvm::contract::exchange::OrderInfo& vo) {
		auto obj = var.get_object();
		from_variant(obj["purchaseAsset"], vo.purchaseAsset);
		from_variant(obj["purchaseNum"], vo.purchaseNum);
		from_variant(obj["payAsset"], vo.payAsset);
		from_variant(obj["payNum"], vo.payNum);
		from_variant(obj["nonce"], vo.nonce);
		from_variant(obj["relayer"], vo.relayer);
		from_variant(obj["fee"], vo.fee);
		from_variant(obj["type"], vo.type);
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
		from_variant(obj["getNum"], vo.getNum);
		from_variant(obj["spentNum"], vo.spentNum);
	}
	void from_variant(const variant& var, uvm::contract::exchange::MatchInfo& vo) {
		auto obj = var.get_object();
		from_variant(obj["fillTakerOrder"], vo.fillTakerOrder);
		from_variant(obj["fillMakerOrders"], vo.fillMakerOrders);
	}
}

namespace uvm {
	namespace contract {
		using namespace cbor_diff;
		using namespace cbor;
		using namespace uvm::contract;
		using uvm::lua::api::global_uvm_chain_api;

		//  contract
		std::string exchange_native_contract::contract_key() const
		{
			return exchange_native_contract::native_contract_key();
		}

		std::set<std::string> exchange_native_contract::apis() const {
			return { "init", "init_config", "fillOrder","cancelOrders","balanceOf","getOrder","withdraw" };
		}
		std::set<std::string> exchange_native_contract::offline_apis() const {
			return { "name", "state", "feeReceiver","balanceOf","getOrder" };
		}
		std::set<std::string> exchange_native_contract::events() const {
			return { "Inited", "OrderCanceled", "OrderFilled","Deposited","Withdrawed" };
		}

		static const std::string not_inited_state_of_exchange_contract = "NOT_INITED";
		static const std::string common_state_of_exchange_contract = "COMMON";

		cbor::CborObjectP order_variant_to_cbor(fc::mutable_variant_object v) {
			CborMapValue order;
			CborMapValue orderInfo;

			if (v.find("orderInfo") == v.end() || (!v["orderInfo"].is_object())) {
				return nullptr;
			}
			const auto& infov = v["orderInfo"].as<fc::mutable_variant_object>();

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

		/*
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
		*/


		void exchange_native_contract::init_api(const std::string& api_name, const std::string& api_arg)
		{
			set_current_contract_storage("name", CborObject::from_string(""));
			set_current_contract_storage("feeReceiver", CborObject::from_string(""));

			// fast map storages: canceledOrders, filledOrders
			set_current_contract_storage("state", CborObject::from_string(not_inited_state_of_exchange_contract));
			const auto& caller_addr = caller_address_string();
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

		static std::string getOrderOwnerAddressAndId(const exchange::Order& o, std::string& addr, std::string& id) {
			const auto& sig_hex = o.sig;
			const auto& infostr = o.orderInfo;
			const auto&  orderinfoDigest = fc::sha256::hash(infostr);
			const auto&  temp = infostr + sig_hex;
			const auto&  orderID = fc::sha256::hash(temp);
			//fc::sha256 orderID(temp);
			id = orderID.str();
			if (o.id != id) {
				printf("id not match,id:%s  caculate_id:%s\n", o.id.c_str(), id.c_str());
				return "id not match";
			}

			std::vector<char> sig_bytes(sig_hex.size() / 2);
			auto bytes_count = fc::from_hex(sig_hex, sig_bytes.data(), sig_bytes.size());
			if (bytes_count != sig_bytes.size()) {
				return "parse sig hex to bytes error";
			}

			fc::ecc::compact_signature compact_sig;
			if (sig_bytes.size() > compact_sig.size())
				return "invalid sig bytes size";
			memcpy(compact_sig.data, sig_bytes.data(), sig_bytes.size());

			const auto& recoved_public_key = fc::ecc::public_key(compact_sig, orderinfoDigest, false);
			if (!recoved_public_key.valid()) {
				return "invalid signature";
			}

			if (!global_uvm_chain_api) {
				return "invalid global_uvm_chain_api";
			}
			else {
				addr = global_uvm_chain_api->pubkey_to_address_string(recoved_public_key);
			}
			return "OK";
		}

		static bool is_integral(std::string number)
		{
			return is_numeric(number.c_str()) && std::strchr(number.c_str(), '.') == 0;
		}

		exchange::OrderInfo exchange_native_contract::checkOrder(const exchange::FillOrder& fillOrder, std::string& addr, std::string& id, std::string& eventOrder,bool& isCompleted) {
			if (getOrderOwnerAddressAndId(fillOrder.order, addr, id) != "OK") {
				throw_error("fillOrder wrong");
			}
			auto orderInfoV = fc::json::from_string(fillOrder.order.orderInfo);
			if (!orderInfoV.is_object()) {
				throw_error("fillOrderInfo not map str");
			}
			exchange::OrderInfo orderInfo;
			fc::from_variant(orderInfoV, orderInfo);

			int64_t purchaseNum = orderInfo.purchaseNum;
			int64_t payNum = orderInfo.payNum;
			int64_t getNum = fillOrder.getNum;
			int64_t spentNum = fillOrder.spentNum;

			if (purchaseNum <= 0 || payNum <= 0 || getNum <= 0 || spentNum <= 0) {
				throw_error("num must > 0");
			}
			if (spentNum > payNum) {
				throw_error("spentNum must < payNum");
			}

			////
			auto feeReceiver = orderInfo.relayer;
			if (feeReceiver == "") {
				feeReceiver = get_string_current_contract_storage("feeReceiver");
			}

			int64_t spentFee = safe_number_to_int64(safe_number_multiply(safe_number_create(spentNum), safe_number_create(orderInfo.fee)));
			if (spentFee < 0) {
				throw_error("spentFee must >= 0");
			}
			if (spentFee >= spentNum) {
				throw_error("spentFee must < spentNum");
			}

			bool isBuyOrder = false;
			if (orderInfo.type == "buy") {
				isBuyOrder = true;
			}
			else if (orderInfo.type == "sell") {
				isBuyOrder = false;
			}
			else {
				throw_error("order type wrong");
			}

			const auto& a = safe_number_multiply(safe_number_create(spentNum), safe_number_create(purchaseNum));
			const auto& b = safe_number_multiply(safe_number_create(getNum), safe_number_create(payNum));
			
			if (safe_number_gt(a, b)) {  
				throw_error("fill order price not satify");
			}

			//check balance
			auto balance = current_fast_map_get(addr, orderInfo.payAsset);
			if (!balance->is_integer()) {
				throw_error("no balance");
			}
			auto bal = balance->force_as_int();
			if (bal < (spentNum+ spentFee)) {   //add fee
				throw_error("not enough balance");
			}

			//check order  canceled , remain num
			auto orderStore = current_fast_map_get(id, "info");
			int64_t lastSpentNum = 0;
			int64_t lastGotNum = 0;
			if (orderStore->is_map()) {
				auto o = orderStore->as_map();
				if (o.find("state") != o.end()) {
					auto state = o["state"]->force_as_int(); //???
					if (state == 0) {
						throw_error("order has been canceled");
					}
					else if (state == 1) {
						throw_error("order has been completely filled");
					}
					else {
						throw_error("wrong order state");
					}
				}
				if (isBuyOrder) {
					if (o.find("gotNum") != o.end()) {
						lastGotNum = o["gotNum"]->force_as_int();
						auto remainNum = purchaseNum - lastGotNum;
						if (remainNum < getNum) {
							throw_error("order remain not enough");
						}
						//write
						o["gotNum"] = CborObject::from_int(lastGotNum + getNum);
						if (o.find("spentdNum") == o.end()) {
							throw_error("wrong order info stored");
						}
						auto lastSpentNum = o["spentdNum"]->force_as_int();
						o["spentdNum"] = CborObject::from_int(lastSpentNum + spentNum);
					}
					else {
						o["gotNum"] = CborObject::from_int(getNum);
						o["spentdNum"] = CborObject::from_int(spentNum);
					}
				}
				else {
					if (o.find("spentdNum") != o.end()) {
						auto lastSpentNum = o["spentdNum"]->force_as_int();
						auto remainNum = payNum - lastSpentNum;
						if (remainNum < spentNum) {
							throw_error("order remain not enough");
						}
						//write
						o["spentdNum"] = CborObject::from_int(lastSpentNum + spentNum);
						if (o.find("gotNum") == o.end()) {
							throw_error("wrong order info stored");
						}
						lastGotNum = o["gotNum"]->force_as_int();
						o["gotNum"] = CborObject::from_int(lastGotNum + getNum);
					}
					else {
						o["gotNum"] = CborObject::from_int(getNum);
						o["spentdNum"] = CborObject::from_int(spentNum);
					}
				}
				current_fast_map_set(id, "info", orderStore);
			}
			else {
				CborMapValue m;
				m["gotNum"] = CborObject::from_int(getNum);
				m["spentNum"] = CborObject::from_int(spentNum);
				current_fast_map_set(id, "info", CborObject::create_map(m));
			}

			////write bal
			current_fast_map_set(addr, orderInfo.payAsset, CborObject::from_int(bal - spentNum - spentFee));
			balance = current_fast_map_get(addr, orderInfo.purchaseAsset);
			if (!balance->is_integer()) {
				bal = 0;
			}
			else {
				bal = balance->force_as_int();
			}
			current_fast_map_set(addr, orderInfo.purchaseAsset, CborObject::from_int(bal + getNum));
			//fee
			if (spentFee > 0) {
				current_fast_map_set(feeReceiver, orderInfo.payAsset, CborObject::from_int(spentFee));
			}

			std::stringstream ss;
			int64_t baseNum = 0;

			if (isBuyOrder) {
				baseNum = purchaseNum - getNum - lastGotNum;
				ss << baseNum << "," << (payNum - spentNum - lastSpentNum) << "," << addr << "," << id << "," << (lastGotNum + getNum) << "," << (lastSpentNum + spentNum) << "," << getNum << "," << spentNum <<","<< spentFee;
				}
			else {
				baseNum = payNum - spentNum - lastSpentNum;
				ss << baseNum << "," << (purchaseNum - getNum - lastGotNum) << "," << addr << "," << id << "," << (lastSpentNum + spentNum) << "," << (lastGotNum + getNum) << "," << spentNum << "," << getNum << "," << spentFee;
				}
			
			if (baseNum <= 0) {
				isCompleted = true;
			}
			else {
				isCompleted = false;
			}
			eventOrder = ss.str();
			return orderInfo;
		}

		void exchange_native_contract::checkMatchedOrders(exchange::FillOrder& takerFillOrder, std::vector<exchange::FillOrder>& makerFillOrders) {
			std::string takerAddr;
			std::string takerOrderId;
			std::string takerEventOrder;
			bool isCompleted = false;
			const auto& orderInfo = checkOrder(takerFillOrder, takerAddr, takerOrderId, takerEventOrder, isCompleted);

			const auto& asset1 = orderInfo.purchaseAsset;
			const auto& asset2 = orderInfo.payAsset;

			const auto& takerOrderType = orderInfo.type;

			int64_t purchaseNum = orderInfo.purchaseNum;
			int64_t payNum = orderInfo.payNum;

			int64_t takerGetNum = takerFillOrder.getNum;
			int64_t takerSpentNum = takerFillOrder.spentNum;

			int64_t totalMakerGetNum = 0;
			int64_t totalMakerSpentNum = 0;

			std::string takerType = orderInfo.type;
			/*
			{"OrderType":"sell","putOnOrder":"128,5,test11,2a96ff7713b7e8c6e34dab2
			de5c2a5844bbc60435b12da5495324d19f0d61853","exchangPair":"HC/COIN","transactionB
			uys":["0,0,test12,bd3bbc9fbc9a09eb43fd952cac840104f4fa0c9f54795e14af8938fff5a10f
			de,125,6,125,6"],"transactionSells":["0,0,test11,2a96ff7713b7e8c6e34dab2de5c2a5
			844bbc60435b12da5495324d19f0d61853,125,6,125,6"],"transactionResult":"FULL_COM
			PLETED","totalExchangeBaseAmount":125,"totalExchangeQuoteAmount":6}
			*/
			bool takerIsBuy = false;
			if (takerOrderType == "buy") {
				takerIsBuy = true;
			}
			jsondiff::JsonObject event_arg;
			jsondiff::JsonArray transactionBuys;
			jsondiff::JsonArray transactionSells;
			event_arg["OrderType"] = takerOrderType;
			//char temp[1024] = {0};
			std::string eventName;
			std::stringstream ss;
			if (takerIsBuy) {
				eventName = "BuyOrderPutedOn";
				transactionBuys.push_back(takerEventOrder);
				event_arg["totalExchangeBaseAmount"] = takerGetNum;
				event_arg["totalExchangeQuoteAmount"] = takerSpentNum;
				event_arg["exchangPair"] = orderInfo.purchaseAsset+","+ orderInfo.payAsset;
				ss << orderInfo.purchaseNum << "," << orderInfo.payNum << "," << takerAddr << "," << takerOrderId;
				}
			else {
				eventName = "SellOrderPutedOn";
				transactionSells.push_back(takerEventOrder);
				event_arg["totalExchangeBaseAmount"] = takerSpentNum;
				event_arg["totalExchangeQuoteAmount"] = takerGetNum;
				event_arg["exchangPair"] = orderInfo.payAsset + "," + orderInfo.purchaseAsset;
				ss << orderInfo.payNum << "," << orderInfo.purchaseNum << "," << takerAddr << "," << takerOrderId;
			}
			event_arg["putOnOrder"] = ss.str();
			event_arg["transactionResult"] = isCompleted?"FULL_COMPLETED":"PARTLY_COMPLETED";

			for (auto it = makerFillOrders.begin(); it != makerFillOrders.end(); it++) {
				std::string address;
				std::string id;
				std::string eventOrder;
				bool isCompleted = false;
				const auto& orderInfo = checkOrder(*it, address, id, eventOrder, isCompleted);

				if (asset1 != orderInfo.payAsset || asset2 != orderInfo.purchaseAsset) {
					throw_error("asset not match");
				}
				if (takerOrderType == orderInfo.type) {
					throw_error("order type not match");
				}

				totalMakerGetNum = it->getNum + totalMakerGetNum;
				totalMakerSpentNum = it->spentNum + totalMakerSpentNum;

				if (takerIsBuy) {
					transactionSells.push_back(eventOrder);
				}
				else {
					transactionBuys.push_back(eventOrder);
				}
			}

			if (totalMakerGetNum != takerSpentNum || totalMakerSpentNum != takerGetNum) {
				throw_error("total exchange num not match");
			}
			event_arg["transactionSells"] = transactionSells;
			event_arg["transactionBuys"] = transactionBuys;
			const auto& argstr = uvm::util::json_ordered_dumps(event_arg);
			printf("%s",argstr.c_str());
			emit_event(eventName, uvm::util::json_ordered_dumps(event_arg));
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
			auto feeReceiver = parsed_args[0];
			boost::trim(feeReceiver);
			auto percentage = parsed_args[1];
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

		//order: {orderInfo:{purchaseAsset:"HC",purchaseNum:20,payAsset:"HX",payNum:100,nounce:"13df",relayer:"HXsrsfsfe3",fee:0.0001},sig:"rsowor233"}
		//{takerOrder:{},takerPayNum:80,fillOrders:[{order:"order_hex_str",fillNum:30},{order:order2,fillNum:50}]
		void exchange_native_contract::fillOrder_api(const std::string& api_name, const std::string& api_args_utf8)
		{
			if (get_storage_state() != common_state_of_exchange_contract)
				throw_error("this exchange contract state is not common");

			auto args = fc::json::from_string(api_args_utf8);
			if (!args.is_object()) {
				throw_error("args not map");
			}
			exchange::MatchInfo matchinfo;
			fc::from_variant(args, matchinfo);
			checkMatchedOrders(matchinfo.fillTakerOrder, matchinfo.fillMakerOrders);
			return;
		}

		void exchange_native_contract::cancelOrders_api(const std::string& api_name, const std::string& api_args) {
			std::vector<std::string> canceledOrderIds;
			
			auto args = fc::json::from_string(api_args);
			if (!args.is_array()) {
				throw_error("args not array");
			}
			std::vector<exchange::Order> orders;

			fc::from_variant(args, orders);
			auto callerAddr = caller_address_string();

			for (int i = 0; i < orders.size(); i++) {
				std::string addr;
				std::string id;
				if (getOrderOwnerAddressAndId(orders[i], addr, id) == "OK") {
					if (callerAddr == addr) {
						//check order  canceled , remain num
						auto orderInfo = current_fast_map_get(id, "info");
						if (orderInfo->is_map()) {
							auto o = orderInfo->as_map();
							if (o.find("state") != o.end()) {
								auto state = o["state"]->force_as_int();
								if (state == 0) { //0 ---- canceled
									continue;
								}
							}

							o["state"] = CborObject::from_int(0);
							current_fast_map_set(id, "info", orderInfo);
							canceledOrderIds.push_back(id);
						}
						else {
							CborMapValue m;
							m["state"] = CborObject::from_int(0);
							current_fast_map_set(id, "info", CborObject::create_map(m));
							canceledOrderIds.push_back(id);
						}
					}
				}
			}
			const auto& result = fc::json::to_string(canceledOrderIds);
			set_api_result(result);
			return;
		}

		void exchange_native_contract::on_deposit_asset_api(const std::string& api_name, const std::string& api_args)
		{

			auto args = fc::json::from_string(api_args);
			if (!args.is_object()) {
				throw_error("args not map");
			}

			auto arginfo = args.as<fc::mutable_variant_object>();
			auto amount = arginfo["num"].as_int64();
			auto symbol = arginfo["symbol"].as_string();
			if (amount <= 0) {
				throw_error("amount must > 0");
			}

			const auto& addr = caller_address_string();

			//check balance
			auto balance = current_fast_map_get(addr, symbol);
			int64_t bal = 0;
			if (balance->is_integer()) {
				bal = balance->force_as_int();
			}
			current_fast_map_set(addr, symbol, CborObject::from_int(bal + amount));

			jsondiff::JsonObject event_arg;
			event_arg["from_address"] = addr;
			event_arg["symbol"] = symbol;
			event_arg["amount"] = amount;
			emit_event("Deposited", uvm::util::json_ordered_dumps(event_arg));
		}

		//args:amount,symbol
		void exchange_native_contract::withdraw_api(const std::string& api_name, const std::string& api_arg)
		{
			std::vector<std::string> parsed_args;
			boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
			if (parsed_args.size() != 2)
				throw_error("argument format error, need format: amount,symbol");

			if (!is_integral(parsed_args[0]))
				throw_error("argument format error, amount must be integral");
		
			int64_t amount = fc::to_int64(parsed_args[0]);
			
			if (amount <= 0) {
				throw_error("amount must > 0");
			}
			const auto& symbol = parsed_args[1];
			if (symbol.empty()) {
				throw_error("symbol is empty");
			}
			const auto& caller = caller_address_string();
			auto balance = current_fast_map_get(caller, symbol);
			int64_t bal = 0;
			if (!balance->is_integer()) {
				throw_error("no balance to withdraw");
			}
			else {
				bal = balance->force_as_int();
			}
			if (amount > bal) {
				throw_error("not enough balance to withdraw");
			}
			auto newBalance = bal - amount;
			if (newBalance == 0) {
				current_fast_map_set(caller, parsed_args[1], CborObject::create_null());
			}
			else {
				current_fast_map_set(caller, parsed_args[1], CborObject::from_int(newBalance));
			}

			current_transfer_to_address(caller, symbol, amount);
			//{"amount":35000,"realAmount":34996,"fee":4,"symbol":"HC","to_address":"test11"}
			jsondiff::JsonObject event_arg;
			event_arg["symbol"] = symbol;
			event_arg["to_address"] = caller;
			event_arg["realAmount"] = amount;
			event_arg["amount"] = amount;
			emit_event("Withdrawed", uvm::util::json_ordered_dumps(event_arg));
		}


		//args: address,symbol
		void exchange_native_contract::balanceOf_api(const std::string& api_name, const std::string& api_arg)
		{
			std::vector<std::string> parsed_args;
			boost::split(parsed_args, api_arg, [](char c) {return c == ','; });
			if (parsed_args.size() != 2)
				throw_error("argument format error, need format: address,symbol");
			auto balance = current_fast_map_get(parsed_args[0], parsed_args[1]);
			int64_t bal = 0;
			if (!balance->is_integer()) {
				bal = 0;
			}
			else {
				bal = balance->force_as_int();
			}
			set_api_result(std::to_string(bal));
			return;
		}

		////args: orderId
		void exchange_native_contract::getOrder_api(const std::string& api_name, const std::string& orderId)
		{
			auto info = current_fast_map_get(orderId, "info");
			if (info->is_map()) {
				set_api_result(fc::json::to_string(info->to_json()));
			}
			else {
				set_api_result("{}");
			}
			return;
		}

		void exchange_native_contract::invoke(const std::string& api_name, const std::string& api_arg) {
			std::map<std::string, std::function<void(const std::string&, const std::string&)>> apis = {
				{ "init", std::bind(&exchange_native_contract::init_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "init_config", std::bind(&exchange_native_contract::init_config_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "fillOrder", std::bind(&exchange_native_contract::fillOrder_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "cancelOrders", std::bind(&exchange_native_contract::cancelOrders_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "balanceOf", std::bind(&exchange_native_contract::balanceOf_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "getOrder", std::bind(&exchange_native_contract::getOrder_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "state", std::bind(&exchange_native_contract::state_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "on_deposit_asset", std::bind(&exchange_native_contract::on_deposit_asset_api, this, std::placeholders::_1, std::placeholders::_2) },
			{ "withdraw", std::bind(&exchange_native_contract::withdraw_api, this, std::placeholders::_1, std::placeholders::_2) },
			};
			if (apis.find(api_name) != apis.end())
			{
				apis[api_name](api_name, api_arg);
				set_invoke_result_caller();
				add_gas(gas_count_for_api_invoke(api_name));
				return;
			}
			throw_error("exchange api not found");
		}
	}
}


