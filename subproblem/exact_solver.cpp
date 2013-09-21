//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <subproblem/exact_solver.h>

vector<Solution> ExactSolver::solve() const {
    vector<Solution> sols;
    
    vector<Path> optimal_paths;
    vector<Label> optimal_labels;
    
    NodeIdFunctor nf(g);
    ArcIdFunctor af(g);
    
    std::shared_ptr<VesselClass> vc = g.vessel_class;
    PortDuals pd = g.port_duals;
    float vd = g.vc_dual;
    
    clock_t cl_start = clock();
    
    r_c_shortest_paths(
        g.graph,
        make_property_map<Vertex>(nf),
        make_property_map<Edge>(af),
        g.h1().second,
        g.h2().second,
        optimal_paths,
        optimal_labels,
        Label(vc->capacity, vc->capacity, 0, 0, pd, vd, g.n_port_ub()),
        LabelExtender(),
        Dominance(),
        allocator<r_c_shortest_paths_label<BGraph, Label>>(),
        default_r_c_shortest_paths_visitor()
    );
        
    clock_t cl_end = clock();
    cout << "Time elapsed (on complete graph): " << (double(cl_end - cl_start) / CLOCKS_PER_SEC) << " seconds." << endl;
        
    for(int i = 0; i < optimal_paths.size(); i++) {
        sols.push_back(Solution(optimal_paths[i], g.calculate_cost(optimal_paths[i]), optimal_labels[i].cost, vc));
    }
    
    return sols;
}