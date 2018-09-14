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

	struct transfer_operation : public generic_operation {
		typedef void_evaluate_result result_type;

		operation_type_enum type;
		std::string from_address;
		std::string to_address;
		asset_id_t asset_id;
		share_type amount;
		fc::time_point_sec op_time;

		transfer_operation() : type(operation_type_enum::TRANSFER) {}
		virtual ~transfer_operation() {}

		virtual operation_type_enum get_type() const {
			return type;
		}
	};
}

FC_REFLECT(simplechain::mint_operation, (type)(account_address)(asset_id)(amount)(op_time))
FC_REFLECT(simplechain::transfer_operation, (type)(from_address)(to_address)(asset_id)(amount)(op_time))