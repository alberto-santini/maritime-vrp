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

#define PCT_START 0.1
#define PCT_END_ELEM 0.6
#define PCT_END 1
#define PCT_INCREMENT 0.1

class SPSolver {
public:
    const std::shared_ptr<const Problem> prob;
    const GraphMap& local_graphs;
    
    SPSolver(const std::shared_ptr<const Problem> prob, const GraphMap& local_graphs) : prob(prob), local_graphs(local_graphs) {}
    
    /*  Returns true if new columns were added to the pool or false otherwise */
    pair<int, ColumnOrigin> solve(ColumnPool& node_pool, std::shared_ptr<ColumnPool> global_pool, const bool try_elementary, double& time_spent_by_exact_solver) const;
};

#endif