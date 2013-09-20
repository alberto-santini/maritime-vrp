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
    const Problem&  prob;
    
    SPSolver(const Problem& prob) : prob(prob) {}
    
    /*  Returns true if new columns were added to the pool or false otherwise */
    bool solve(ColumnPool& pool);
};

#endif