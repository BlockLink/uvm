#include <simplechain/contract_object.h>
#include <simplechain/contract_entry.h>

namespace simplechain {
	ContractEntryPrintable contract_object::to_printable(const contract_object& obj) {
		ContractEntryPrintable printable;
		printable.id = obj.contract_address;
		printable.owner_address = obj.owner_address;
		printable.name = obj.contract_name;
		printable.description = obj.contract_desc;
		printable.type_of_contract = obj.type_of_contract;
		printable.code_printable = obj.code;
		printable.registered_block = obj.registered_block;
		if (obj.inherit_from != "")
		{
			printable.inherit_from = obj.inherit_from;
		}
		for (auto& addr : obj.derived)
		{
			printable.derived.push_back(addr);
		}
		printable.createtime = obj.create_time;
		return printable;
	}
}

namespace fc {
	void to_variant(const simplechain::contract_object& var, variant& vo) {
		fc::mutable_variant_object obj("registered_block", var.registered_block);
		obj("owner_address", var.owner_address);
		// TODO: code to varient
		obj("create_time", var.create_time);
		obj("contract_address", var.contract_address);
		obj("contract_name", var.contract_name);
		obj("contract_desc", var.contract_desc);
		obj("type_of_contract", var.type_of_contract);
		obj("native_contract_key", var.native_contract_key);
		obj("derived", var.derived);
		obj("inherit_from", var.inherit_from);
		vo = std::move(obj);
	}
}