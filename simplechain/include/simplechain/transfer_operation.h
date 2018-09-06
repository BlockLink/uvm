#pragma once
#include <simplechain/operation.h>
#include <simplechain/evaluate_result.h>
#include <simplechain/asset.h>

namespace simplechain {
	struct mint_operation : public generic_operation {
		typedef void_evaluate_result result_type;

		operation_type_enum type;
		std::string account_address;
		asset_id_t asset_id;
		share_type amount;
		fc::time_point_sec op_time;

		mint_operation() : type(operation_type_enum::MINT) {}
		virtual ~mint_operation() {}

		virtual operation_type_enum get_type() const {
			return type;
		}
	};
}

FC_REFLECT(simplechain::mint_operation, (type)(account_address)(asset_id)(amount)(op_time))