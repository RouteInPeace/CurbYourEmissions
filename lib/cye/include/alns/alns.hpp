#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace alns {

template <typename Solution, typename Instance, typename AcceptanceCriterion, typename OperatorSelection>
class ALNS {

public:
    struct Config {
        size_t max_iterations;
    };

    ALNS(std::shared_ptr<Instance> instance, Solution &initial_solution, AcceptanceCriterion &&acceptance_criterion,
         OperatorSelection &&operator_selection)
        : instance(instance), current_solution_(initial_solution), best_solution_(initial_solution), 
        acceptance_criterion_(std::move(acceptance_criterion)), operator_selection_(std::move(operator_selection)) {}

    ALNS() = default;

    void run(Solution& initial_solution, Config config) {
        current_solution_ = initial_solution;
        best_solution_ = initial_solution;

        for (size_t i = 0; i < config.max_iterations; ++i) {
            auto destroy_operator_id = operator_selection_.select_operator();
            auto repair_operator_id = operator_selection_.select_operator();

            auto destroyed_solution = destroy_operators[destroy_operator_id](current_solution_);
            auto repaired_solution = repair_operators[repair_operator_id](destroyed_solution);

            auto new_cost = repaired_solution.get_cost();
            auto old_cost = current_solution_.get_cost();
            auto best_cost = best_solution_.get_cost();

            if (acceptance_criterion_.accept(new_cost, old_cost, best_cost)) {
                current_solution_ = repaired_solution;
                if (new_cost < best_cost) {
                    best_solution_ = repaired_solution;
                }
            }
            operator_selection_.update(new_cost, old_cost, best_cost);
        }
    }


    void add_destroy_operator(std::function<Solution(Solution&)> destroy_operator) {
        destroy_operators.push_back(destroy_operator);
    }
    void add_repair_operator(std::function<Solution(Solution&)> repair_operator) {
        repair_operators.push_back(repair_operator);
    }

    Solution get_best_solution() const {
        return best_solution_;
    }

private:
    std::shared_ptr<Instance> instance;
    Solution current_solution_;
    Solution best_solution_;
    AcceptanceCriterion acceptance_criterion_;
    OperatorSelection operator_selection_;
    std::vector<std::function<Solution(Solution&)>> destroy_operators;
    std::vector<std::function<Solution(Solution&)>> repair_operators;
};

};