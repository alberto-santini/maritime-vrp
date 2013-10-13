//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef EXACT_SOLVER_H
#define EXACT_SOLVER_H

#include <base/base.h>
#include <base/graph.h>
#include <column/solution.h>
#include <subproblem/labelling.h>

class ExactSolver {
public:
    const std::shared_ptr<const Graph>    g;
    
    ExactSolver(const std::shared_ptr<const Graph> g) : g(g) {}
    
    vector<Solution> solve() const;
};

#endif