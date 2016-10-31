//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef HEURISTICS_SOLVER_H
#define HEURISTICS_SOLVER_H

#include <memory>

#include "../base/graph.h"
#include "../base/problem.h"
#include "../column/solution.h"
#include "../preprocessing/program_params.h"

namespace mvrp {
    struct HeuristicsSolver {
        std::shared_ptr<const Problem> prob;
        std::shared_ptr<const Graph> g;
        const ErasedEdges &erased;

        HeuristicsSolver(std::shared_ptr<const Problem> prob, std::shared_ptr<const Graph> g, const ErasedEdges &erased) : prob(prob), g(g), erased{erased} {}

        std::vector<Solution> solve_fast() const;
        std::vector<Solution> solve_elem_on_reduced_graph() const { return solve_elem_on_generic_graph(false); }
        std::vector<Solution> solve_on_reduced_graph() const { return solve_on_generic_graph(false); }
        std::vector<Solution> solve_elem_on_smart_graph() const { return solve_elem_on_generic_graph(true); }
        std::vector<Solution> solve_on_smart_graph() const { return solve_on_generic_graph(true); }

    private:
        std::vector<Solution> solve_fast_forward() const;
        std::vector<Solution> solve_fast_backward() const;
        std::vector<Solution> solve_elem_on_generic_graph(bool smart) const;
        std::vector<Solution> solve_on_generic_graph(bool smart) const;
    };
}

#endif
