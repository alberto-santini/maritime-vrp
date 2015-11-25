//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef MP_SOLVER_H
#define MP_SOLVER_H

#include <cstring>
#include <memory>
#include <utility>

#include <ilcplex/ilocplex.h>

#include <base/problem.h>
#include <column/column_pool.h>
#include <masterproblem/mp_linear_solution.h>
#include <masterproblem/mp_integer_solution.h>

typedef std::tuple<IloEnv, IloNumVarArray, IloRangeArray, IloRangeArray, IloCplex> IloData;

class MPSolver {
public:
    std::shared_ptr<const Problem> prob;

    MPSolver(std::shared_ptr<const Problem> prob) : prob(prob) {}
    
    MPLinearSolution solve_lp(const ColumnPool& pool) const;
    MPIntegerSolution solve_mip(const ColumnPool& pool) const;
    
private:
    IloData solve(const ColumnPool& pool, bool linear) const;
};

#endif