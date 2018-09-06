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