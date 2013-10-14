//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <subproblem/threaded_solver.h>

inline bool solution_in_pool(const Solution& s, const ColumnPool& pool) {
    return (find_if(pool.begin(), pool.end(),
    [&s] (const Column& c) {
        return (s == c.sol);
    }) != pool.end());
}

void ThreadedSolver::solve_fast(std::shared_ptr<const Problem> prob, std::shared_ptr<Graph> g, std::shared_ptr<ColumnPool> node_pool, std::shared_ptr<std::vector<Solution>> valid_sols, int& discarded_prc, int& discarded_infeasible, int& discarded_generated, int& discarded_in_pool) {
    HeuristicsSolver hsolv(prob->params, g);    
    vector<Solution> total = hsolv.solve_fast();
    
    lock_guard<mutex> grd(mtx);
    for(const Solution& s : total) {
        if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
            discarded_prc++;
        } else if(!s.satisfies_capacity_constraints()) {
            discarded_infeasible++;
        } else if(find(valid_sols->begin(), valid_sols->end(), s) != valid_sols->end()) {
            discarded_generated++;
        } else if(solution_in_pool(s, *node_pool)) {
            discarded_in_pool++;
        } else {
            valid_sols->push_back(s);
        }
    }
}

void ThreadedSolver::solve_on_reduced_graph(std::shared_ptr<const Problem> prob, std::shared_ptr<Graph> g, const float& lambda, std::shared_ptr<ColumnPool> node_pool, std::shared_ptr<std::vector<Solution>> valid_sols, int& discarded_prc, int& discarded_infeasible, int& discarded_generated, int& discarded_in_pool) {
    HeuristicsSolver hsolv(prob->params, g);
    vector<Solution> red_sols = hsolv.solve_on_reduced_graph(lambda);
    
    lock_guard<mutex> grd(mtx);
    for(const Solution& s : red_sols) {
        if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
            discarded_prc++;
        } else if(find(valid_sols->begin(), valid_sols->end(), s) != valid_sols->end()) {
            discarded_generated++;
        } else if(solution_in_pool(s, *node_pool)) {
            discarded_in_pool++;
        } else {
            valid_sols->push_back(s);
        }
    }
}

void ThreadedSolver::solve_on_complete_graph(std::shared_ptr<Graph> g, std::shared_ptr<ColumnPool> node_pool, std::shared_ptr<std::vector<Solution>> valid_sols, int& discarded_prc, int& discarded_infeasible, int& discarded_generated, int& discarded_in_pool) {
    ExactSolver esolv(g);
    vector<Solution> e_sols = esolv.solve();
    
    lock_guard<mutex> grd(mtx);
    for(const Solution& s : e_sols) {        
        if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
            discarded_prc++;
        } else if(find(valid_sols->begin(), valid_sols->end(), s) != valid_sols->end()) {
            discarded_generated++;
        } else if(solution_in_pool(s, *node_pool)) {
            discarded_in_pool++;
        } else {
            valid_sols->push_back(s);
        }
    }    
}