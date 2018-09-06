#include <simplechain/transfer_evaluate.h>
#include <simplechain/blockchain.h>

namespace simplechain {
	std::shared_ptr<min_operation_evaluator::operation_type::result_type> min_operation_evaluator::do_evaluate(const operation_type& op) {
		auto asset_item = get_chain()->get_asset(op.asset_id);
		FC_ASSERT(asset_item);
		FC_ASSERT(op.amount > 0);
		// TODO: check address format
		return std::make_shared<void_evaluate_result>();
	}
	std::shared_ptr<min_operation_evaluator::operation_type::result_type> min_operation_evaluator::do_apply(const operation_type& op) {
		auto result = do_evaluate(op);
		get_chain()->update_account_asset_balance(op.account_address, op.asset_id, op.amount);
		return result;
	}
}