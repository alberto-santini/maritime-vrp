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
        } else {
            throw runtime_error("The hub is not the first port!");
        }
    }
    
    vc_coeff = vector<float>(nv, 0);
    for(int i = 0; i < nv; i++) {
        if(sol.vessel_class == prob.data.vessel_classes[i]) {
            vc_coeff[i] = 1;
        }
    }
    
    dummy = false;
}

void Column::make_dummy(const float huge_cost) {
    solution = Solution();
    obj_coeff = huge_cost;
    port_coeff = vector<float>(2 * (prob.data.num_ports - 1), 1);
    vc_coeff = vector<float>(prob.data.num_vessel_classes, 0);
    dummy = true;
}

bool Column::is_compatible_with_unite_rule(VisitRule vr) const {
    Graph& g = prob.graphs.at(solution.vessel_class);
    
    for(Edge e : solution.path) {
        Node orig = *g.graph[source(e, g.graph)];
        Node dest = *g.graph[target(e, g.graph)];
        
        /*  If orig~vr.first and !dest~vr.second OR
               !orig~vr.first and dest~vr.second
            then the path is not compatible! */
        if(orig.same_row_as(*vr.first) != dest.same_row_as(*vr.second)) {
            return false;
        }
    }
    
    return true;
}

bool Column::is_compatible_with_separate_rule(VisitRule vr) const {
    Graph& g = prob.graphs.at(solution.vessel_class);
    
    for(Edge e : solution.path) {
        Node orig = *g.graph[source(e, g.graph)];
        Node dest = *g.graph[target(e, g.graph)];
        
        /*  If orig~vr.first and dest~vr.second then the path is not compatible! */
        if(orig.same_row_as(*vr.first) && dest.same_row_as(*vr.second)) {
            return false;
        }
    }
    
    return true;
}