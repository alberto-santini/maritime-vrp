//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef THREADED_SOLVER
#define THREADED_SOLVER

#include <base/base.h>
#include <column/column_pool.h>
#include <subproblem/exact_solver.h>
#include <subproblem/heuristics_solver.h>

class ThreadedSolver {
public:
    mutex mtx;
    
    ThreadedSolver() {}
    
    void solve_fast(std::shared_ptr<const Problem> prob, std::shared_ptr<Graph> g, std::shared_ptr<ColumnPool> node_pool, std::shared_ptr<std::vector<Solution>> valid_sols, int& discarded_prc, int& discarded_infeasible, int& discarded_generated, int& discarded_in_pool);
    
    void solve_on_reduced_graph(std::shared_ptr<const Problem> prob, std::shared_ptr<Graph> g, const float& lambda, std::shared_ptr<ColumnPool> node_pool, std::shared_ptr<std::vector<Solution>> valid_sols, int& discarded_prc, int& discarded_infeasible, int& discarded_generated, int& discarded_in_pool);
    
    void solve_on_complete_graph(std::shared_ptr<Graph> g, std::shared_ptr<ColumnPool> node_pool, std::shared_ptr<std::vector<Solution>> valid_sols, int& discarded_prc, int& discarded_infeasible, int& discarded_generated, int& discarded_in_pool);
};

#endif