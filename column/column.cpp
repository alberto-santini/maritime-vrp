//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <column/column.h>

Column::Column(const Problem& prob, const Solution sol) : prob(prob), sol(sol) {
    obj_coeff = sol.cost;
    
    Graph& g = prob.graphs.at(sol.vessel_class);
    int np = prob.data.num_ports;
    int nv = prob.data.num_vessel_classes;
    
    /*  "np - 1" to remove the hub;
        "2 *" to create one coefficient for (port, pu) and one for (port, de) */
    port_coeff = vector<float>(2 * (np - 1), 0);
    for(Edge e : sol.path) {
        Node n = *g.graph[target(e, g.graph)];
        if(n.n_type == NodeType::REGULAR_PORT) {
            for(int i = 1; i < np; i++) {
                if(n.port == prob.data.ports[i]) {
                    int constr_index = (n.pu_type == PickupType::PICKUP ? (i - 1) : (np - 1 + i - 1));
                    prob_coeff[constr_index]++;
                }
            }
        }
    }
    
    vc_coeff = vector<float>(nv, 0);
    for(int i = 0; i < nv; i++) {
        if(sol.vessel_class == prob.data.vessel_classes[i]) {
            vc_coeff[i] = 1;
        }
    }
}