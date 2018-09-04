#pragma once
#include <simplechain/evaluator.h>
#include <simplechain/contract.h>
#include <simplechain/evaluate_state.h>

namespace simplechain {
	class contract_create_evaluator : public evaluator<contract_create_evaluator>, public evaluate_state {
	public:
		typedef contract_create_operation operation_type;
	public:
		contract_create_evaluator(blockchain* chain_, transaction* tx_): evaluate_state(chain_, tx_){}

		virtual std::shared_ptr<operation_type::result_type> do_evaluate(const operation_type& op) final;
		virtual std::shared_ptr<operation_type::result_type> do_apply(const operation_type& op) final;

	private:
		void undo_contract_effected();
	};

	class contract_invoke_evaluator : public evaluator<contract_invoke_evaluator>, public evaluate_state {
	public:
		typedef contract_invoke_operation operation_type;
	public:
		contract_invoke_evaluator(blockchain* chain_, transaction* tx_) : evaluate_state(chain_, tx_) {}

		virtual std::shared_ptr<operation_type::result_type> do_evaluate(const operation_type& op) final;
		virtual std::shared_ptr<operation_type::result_type> do_apply(const operation_type& op) final;

	private:
		void undo_contract_effected();
	};
}