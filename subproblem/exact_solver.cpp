//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <subproblem/exact_solver.h>

vector<Solution> ExactSolver::solve(double& time_spent) const {
    vector<Solution> sols;
    
    vector<Path> optimal_paths;
    vector<Label> optimal_labels;
    
    NodeIdFunctor nf(g);
    ArcIdFunctor af(g);
    
    std::shared_ptr<VesselClass> vc = g->vessel_class;
    
    clock_t cl_start = clock();
    
    r_c_shortest_paths(
        g->graph,
        make_property_map<Vertex>(nf),
        make_property_map<Edge>(af),
        g->h1().second,
        g->h2().second,
        optimal_paths,
        optimal_labels,
        Label(vc->capacity, vc->capacity, 0),
        LabelExtender(),
        Dominance(),
        allocator<r_c_shortest_paths_label<BGraph, Label>>(),
        default_r_c_shortest_paths_visitor()
    );
        
    clock_t cl_end = clock();
    double time_s = (double(cl_end - cl_start) / CLOCKS_PER_SEC);
    if(time_s - time_spent > numeric_limits<double>::epsilon()) {
        time_spent = time_s;
    }
        
    for(int i = 0; i < optimal_paths.size(); i++) {
        sols.push_back(Solution(optimal_paths[i], g->calculate_cost(optimal_paths[i]), optimal_labels[i].cost, vc, g));
    }
    
    return sols;
}