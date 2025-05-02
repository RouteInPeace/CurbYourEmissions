#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "acceptance_criterion.hpp"
#include "operator_selection.hpp"

namespace alns {

template <typename Solution, typename Instance>
class ALNS {

public:
    struct Config {
        AcceptanceCriterion *acceptance_criterion;
        OperatorSelection *operator_selection;
        size_t max_iterations;
    };

    ALNS(std::shared_ptr<Instance> instance, Solution &initial_solution)
        : instance(instance), current_solution_(initial_solution), best_solution_(initial_solution) {}

    ALNS() = default;

    void run(Solution& initial_solution, Config config) {
        current_solution_ = initial_solution;
        best_solution_ = initial_solution;

        for (int i = 0; i < config.max_iterations; ++i) {
            auto destroy_operator_id = config.operator_selection->select_operator();
            auto repair_operator_id = config.operator_selection->select_operator();

            auto destroyed_solution = destroy_operators[destroy_operator_id](current_solution_);
            auto repaired_solution = repair_operators[repair_operator_id](destroyed_solution);

            auto new_cost = repaired_solution.get_cost();
            auto old_cost = current_solution_.get_cost();
            auto best_cost = best_solution_.get_cost();

            if (config.acceptance_criterion->accept(new_cost, old_cost, best_cost)) {
                current_solution_ = repaired_solution;
                if (new_cost < best_cost) {
                    best_solution_ = repaired_solution;
                }
            }
            config.operator_selection->update(new_cost, old_cost, best_cost);
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
    Solution current_solution_;
    Solution best_solution_;
    std::shared_ptr<Instance> instance;

    std::vector<std::function<Solution(Solution&)>> destroy_operators;
    std::vector<std::function<Solution(Solution&)>> repair_operators;
};

};