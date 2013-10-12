//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef SP_SOLVER_H
#define SP_SOLVER_H

#include <base/base.h>
#include <base/problem.h>
#include <column/column_pool.h>
#include <subproblem/heuristics_solver.h>
#include <subproblem/exact_solver.h>

class SPSolver {
public:
    const std::shared_ptr<const Problem> prob;
    const GraphMap& local_graphs;
    
    SPSolver(const std::shared_ptr<const Problem> prob, const GraphMap& local_graphs) : prob(prob), local_graphs(local_graphs) {}
    
    /*  Returns true if new columns were added to the pool or false otherwise */
    int solve(ColumnPool& node_pool, std::shared_ptr<ColumnPool> global_pool);
};

#endif