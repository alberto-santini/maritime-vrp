//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef MP_SOLVER_H
#define MP_SOLVER_H

#include <base/base.h>
#include <base/problem.h>
#include <column/column_pool.h>
#include <masterproblem/mp_linear_solution.h>

class MPSolver {
public:
    const Problem&      prob;

    MPSolver(const Problem& prob) : prob(prob) {}
    
    MPLinearSolution solve_lp(const ColumnPool& pool) const;
};

#endif