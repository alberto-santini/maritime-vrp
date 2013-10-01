//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <branching/bb_node.h>
#include <branching/cycle.h>

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
    
    for(const auto& cc : root_node.base_columns) {
        cout << "Column selected with coefficient " << cc.second << endl;
        prob.graphs.at(cc.first.sol.vessel_class).print_path(cc.first.sol.path);
        Path shortest_cycle = Cycle::shortest_cycle(cc.first.sol.path, prob.graphs.at(cc.first.sol.vessel_class));
        if(!shortest_cycle.empty()) {
            cout << "Path contains at least one cycle. The shortest one is:" << endl;
            prob.graphs.at(cc.first.sol.vessel_class).print_path(shortest_cycle);
        }
    }
    
    root_node.solve_integer();
    float ub = root_node.sol_value;
    
    cout << "Root node UB: " << ub << endl;
    
    cout << "Gap: " << (ub - lb)/lb * 100 << "\%" << endl;
    
    return 0;
}