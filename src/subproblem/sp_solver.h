//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef SP_SOLVER_H
#define SP_SOLVER_H

#include <utility>

#include "../base/problem.h"
#include "../column/column_pool.h"
#include "heuristics_solver.h"
#include "exact_solver.h"

namespace mvrp {
    static constexpr double pct_start = 0.1;
    static constexpr double pct_end_elem = 0.6;
    static constexpr double pct_end = 1.0;
    static constexpr double pct_increment = 0.1;

    class SPSolver {
    public:
        std::shared_ptr<const Problem> prob;
        const ErasedEdgesMap &local_erased_edges;

        SPSolver(std::shared_ptr<const Problem> prob,
                 const ErasedEdgesMap &local_erased_edges) :
            prob(prob),
            local_erased_edges{local_erased_edges} {}

        /* Returns how many columns were added to the column pool and their origin */
        std::pair<int, ColumnOrigin>
        solve(ColumnPool &node_pool, std::shared_ptr<ColumnPool> global_pool, bool try_elementary,
              double &time_spent_by_exact_solver) const;

    private:
        bool solution_in_pool(const Solution &s, const ColumnPool &pool) const;

        void print_report(int sols_found, int discarded_prc, int discarded_infeasible, int discarded_generated,
                          int discarded_in_pool, std::ostream &out = std::cerr) const;
    };
}

#endif