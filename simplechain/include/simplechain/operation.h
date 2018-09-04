#pragma once
#include <simplechain/config.h>

namespace simplechain {

	enum operation_type_enum : int32_t {
		TRANSFER = 0,

		CONTRACT_CREATE = 101,
		CONTRACT_INVOKE = 102
	};

	struct generic_operation {
		virtual ~generic_operation() {};

		virtual operation_type_enum get_type() const = 0;
	};
}

FC_REFLECT_ENUM(simplechain::operation_type_enum, (TRANSFER)(CONTRACT_CREATE)(CONTRACT_INVOKE))