//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef MP_SOLVER_H
#define MP_SOLVER_H

#include <cstring>
#include <memory>
#include <utility>

#include <ilcplex/ilocplex.h>

#include "../base/problem.h"
#include "../column/column_pool.h"
#include "mp_linear_solution.h"
#include "mp_integer_solution.h"

namespace mvrp {
    struct MPSolver {
        std::shared_ptr<const Problem> prob;

        MPSolver(std::shared_ptr<const Problem> prob) : prob(prob) {}

        MPLinearSolution solve_lp(const ColumnPool &pool, const std::vector<PortWithType>& ports_with_equality) const;
        MPIntegerSolution solve_mip(const ColumnPool &pool, const std::vector<PortWithType>& ports_with_equality) const;

    private:
        using IloData = std::tuple<IloEnv, IloNumVarArray, IloRangeArray, IloRangeArray, IloCplex>;
        IloData solve(const ColumnPool &pool, const std::vector<PortWithType>& ports_with_equality, bool linear) const;

        bool should_row_have_equality(int row, const std::vector<PortWithType>& ports_with_equality) const;
    };
}

#endif