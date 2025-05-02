#include <algorithm>
#include <random>
#include <set>

#include <alns/alns.hpp>
#include <heuristics/init_heuristics.hpp>

auto main() -> int { 
    auto instance = std::make_shared<cye::Instance>("dataset/json/E-n22-k4.json");
    auto initial_solution = cye::nearest_neighbor(instance);

    alns::ALNS<cye::Solution, cye::Instance> alns(instance, initial_solution);

    alns.add_destroy_operator([&](cye::Solution &solution) {
        int n_to_remove = rand() % instance->customer_cnt();
        std::set <size_t> removed_ids;
        auto customer_ids = instance->customer_ids();
        std::sample(customer_ids.begin(), customer_ids.end(), 
        std::inserter(removed_ids, removed_ids.end()), 
            n_to_remove, std::mt19937{std::random_device{}()});

        auto &routes = solution.routes();
        std::vector<size_t> new_routes;
        for (auto id : routes) {
            if (removed_ids.find(id) == removed_ids.end()) {
                new_routes.push_back(id);
            }
        }
        auto unassigned = std::vector<size_t>(removed_ids.begin(), removed_ids.end());
        auto destroyed_solution = cye::Solution(instance, std::move(new_routes), std::move(unassigned));
        return destroyed_solution;
    });
    
    alns.add_repair_operator([](cye::Solution &solution) {
        
        // TODO

        return solution;
    });
    
    alns.run(initial_solution, {
        .acceptance_criterion = new alns::HillClimbingCriterion(),
        .operator_selection = new alns::RandomOperatorSelection(1),
        .max_iterations = 1000
    });
    
    auto best_solution = alns.get_best_solution();
    
    return 0;
}