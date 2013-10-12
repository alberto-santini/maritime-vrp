//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef MP_SOLVER_H
#define MP_SOLVER_H

#include <base/base.h>
#include <base/problem.h>
#include <column/column_pool.h>
#include <masterproblem/mp_linear_solution.h>
#include <masterproblem/mp_integer_solution.h>

typedef std::tuple<IloEnv, IloNumVarArray, IloRangeArray, IloRangeArray, IloCplex> IloData;

class MPSolver {
public:
    const std::shared_ptr<const Problem> prob;

    MPSolver(const std::shared_ptr<const Problem> prob) : prob(prob) {}
    
    MPLinearSolution solve_lp(const ColumnPool& pool) const;
    MPIntegerSolution solve_mip(const ColumnPool& pool) const;
    
private:
    IloData solve(const ColumnPool& pool, const bool linear) const;
};

#endif