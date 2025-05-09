#include <algorithm>
#include <iostream>
#include <random>
#include <vector>
#include <set>

#include <heuristics/init_heuristics.hpp>

#include "alns.hpp"
#include "acceptance_criterion.hpp"
#include "json_archive.hpp"
#include "operator_selection.hpp"
#include "core/solution.hpp"

auto main() -> int { 
    auto archive = serial::JSONArchive("dataset/json/E-n33-k4.json");
    auto instance = std::make_shared<cye::Instance>(archive.root());
    auto initial_solution = cye::nearest_neighbor(instance);

    alns::ALNS<cye::Solution, cye::Instance, alns::HillClimbingCriterion, alns::RandomOperatorSelection> alns(instance, initial_solution, alns::HillClimbingCriterion(), alns::RandomOperatorSelection(1));
    std::cout << "Initial solution cost: " << initial_solution.get_cost() << std::endl;

    alns.add_destroy_operator([&](cye::Solution const &solution) {
        // Random random destruction
        int n_to_remove = rand() % instance->customer_cnt() / 3;
        std::set <size_t> removed_ids;

        auto customer_ids = std::ranges::to<std::vector<size_t>>(instance->customer_ids());
        std::vector<size_t> unassigned;
        std::sample(customer_ids.begin(), customer_ids.end(),
            std::back_inserter(unassigned), n_to_remove,
            std::mt19937{std::random_device{}()});

        removed_ids.insert(unassigned.begin(), unassigned.end());

        auto &routes = solution.routes();
        std::vector<size_t> new_routes;
        for (auto id : routes) {
            if (removed_ids.find(id) == removed_ids.end()) {
                new_routes.push_back(id);
            }
        }
        std::shuffle(unassigned.begin(), unassigned.end(), std::mt19937{std::random_device{}()});
        auto destroyed_solution = cye::Solution(instance, std::move(new_routes), std::move(unassigned));
        return destroyed_solution;
    });

    alns.add_repair_operator([](cye::Solution const &solution) {
        // Greedy repair
        auto copy = solution;
        auto const &unassigned_ids = copy.unassigned_customers();

        for (auto unassigned_id : unassigned_ids) {
            auto best_cost = std::numeric_limits<double>::max();
            // charging stations can get reordered
            auto best_solution = cye::Solution(nullptr, {});

            auto update_cost = [&](cye::Solution &&new_solution) {
                auto cost = copy.get_cost();
                if (cost < best_cost) {
                    best_cost = cost;
                    best_solution = std::move(new_solution);
                }
            };

            for (size_t i = 1; i < copy.routes().size(); ++i) {
                auto new_copy = copy;
                new_copy.insert_customer(i, unassigned_id);
                if (new_copy.is_energy_and_cargo_valid()) {
                    update_cost(std::move(new_copy));
                    continue;
                }
                if(new_copy.reorder_charging_station(i)) {
                    update_cost(std::move(new_copy));
                }
            }
            if (best_solution.routes().size()) {
                copy = best_solution;
            }
        }

        copy.clear_unassigned_customers();
        return copy;
    });

    alns.run(initial_solution, {
        .max_iterations = 100000
    });
    
    auto best_solution = alns.get_best_solution();
    std::cout << "Best solution cost: " << best_solution.get_cost() << std::endl;
    
    return 0;
}
