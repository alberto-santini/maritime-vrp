//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <branching/bb_node.h>

int main() {
    Problem prob = Problem();
    
    for(auto& vg : prob.graphs) {
        cout << vg.second.name << " has " << num_vertices(vg.second.graph) << " vertices and " << num_edges(vg.second.graph) << " edges" << endl;
    }

    ColumnPool pool;
    Column dummy(prob);
    dummy.make_dummy(999);
    pool.push_back(dummy);
    
    BBNode root_node(prob, prob, pool, pool, VisitRule(), VisitRule());
    
    root_node.solve();
    float lb = root_node.sol_value;

    cout << "Root node LB: " << lb << endl;
    
    root_node.solve_integer();
    float ub = root_node.sol_value;
    
    cout << "Root node UB: " << ub << endl;
    
    cout << "Gap: " << (ub - lb)/lb * 100 << "\%" << endl;
    
    return 0;
}