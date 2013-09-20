//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <masterproblem/mp_solver.h>
#include <subproblem/sp_solver.h>

int main() {
    Problem prob = Problem();
    
    for(auto& vg : prob.graphs) {
        cout << vg.second.name << " has " << num_vertices(vg.second.graph) << " vertices and " << num_edges(vg.second.graph) << " edges" << endl;
    }

    Problem pb_prob = prob;
    ColumnPool pool;
    Column dummy(pb_prob);
    dummy.make_dummy(999);
    pool.push_back(dummy);
    
    MPSolver mp_solv(pb_prob);
    MPLinearSolution sol = mp_solv.solve_lp(pool);
    cout << "Objective value: " << sol.obj_value << endl;
    
    bool optimal = false;
    
    while(!optimal) {
        for(auto& vg : pb_prob.graphs) {
            vg.second.port_duals = sol.port_duals;
            vg.second.vc_dual = sol.vc_duals.at(vg.first);
        }
        
        SPSolver sp_solv(pb_prob);
        
        if(sp_solv.solve(pool)) {
           sol = mp_solv.solve_lp(pool);
           cout << "Objective value: " << sol.obj_value << endl;
        } else {
            bool infeasible = false;
            for(int i = 0; i < sol.variables.size(); i++) {
                if(pool[i].dummy && sol.variables[i] > numeric_limits<float>::epsilon()) {
                    cout << "Infeasible!" << endl;
                    infeasible = true;
                    break;
                }
            }
            
            if(!infeasible) {
                cout << "We are optimal!" << endl;
            }
            
            optimal = true;
        }
    }
    
    return 0;
}