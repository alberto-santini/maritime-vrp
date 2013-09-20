//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <base/base.h>
#include <base/problem.h>
#include <column/solution.h>
#include <subproblem/heuristics_solver.h>
#include <subproblem/exact_solver.h>

int main() {
    Problem prob = Problem();
    
    for(auto& vg : prob.graphs) {
        cout << vg.second.name << " has " << num_vertices(vg.second.graph) << " vertices and " << num_edges(vg.second.graph) << " edges" << endl;
    }

    Graph& g = prob.graphs.at(prob.data.vessel_classes.front());
    for(std::shared_ptr<Port> p : prob.data.ports) {
        g.port_duals.emplace(p, make_pair(0.0, 0.0));
    }

    HeuristicsSolver hsolv(prob.params, g);
    ExactSolver esolv(g);
    vector<Solution> sols;
    
    sols = hsolv.solve_fast_backward();
    
    for(const Solution& s : sols) {
        g.print_path(s.path);
    }
    
    sols = esolv.solve();
    
    for(const Solution& s : sols) {
        g.print_path(s.path);
    }

    return 0;
}