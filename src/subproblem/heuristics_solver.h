//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef HEURISTICS_SOLVER_H
#define HEURISTICS_SOLVER_H

#include <memory>

#include <base/graph.h>
#include <base/problem.h>
#include <column/solution.h>
#include <preprocessing/program_params.h>
#include <subproblem/labelling.h>

class HeuristicsSolver {
public:
    const std::shared_ptr<const Problem> prob;
    const std::shared_ptr<const Graph>  g;
    
    HeuristicsSolver(const std::shared_ptr<const Problem> prob, const std::shared_ptr<const Graph> g) : prob(prob), g(g) {}
    
    std::vector<Solution> solve_fast() const;
    std::vector<Solution> solve_elem_on_reduced_graph(double percentage) const;
    std::vector<Solution> solve_on_reduced_graph(double percentage) const { return solve_on_generic_graph(percentage, false); }
    std::vector<Solution> solve_on_smart_graph() const { return solve_on_generic_graph(0, true); };
    
private:
    std::vector<Solution> solve_fast_forward() const;
    std::vector<Solution> solve_fast_backward() const;
    std::vector<Solution> solve_on_generic_graph(double percentage, bool smart) const;
};

#endif