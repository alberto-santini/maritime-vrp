//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <column/column.h>

Column::Column(const Problem& prob, const Solution sol) : prob(prob), sol(sol) {
    obj_coeff = sol.cost;
    
    const Graph& g = prob.graphs.at(sol.vessel_class);
    int np = prob.data.num_ports;
    int nv = prob.data.num_vessel_classes;
    
    /*  "np - 1" to remove the hub;
        "2 *" to create one coefficient for (port, pu) and one for (port, de) */
    port_coeff = vector<float>(2 * (np - 1), 0);
    for(const Edge& e : sol.path) {
        Node n = *g.graph[target(e, g.graph)];
        if(n.n_type == NodeType::REGULAR_PORT) {
            for(int i = 1; i < np; i++) {
                if(n.port == prob.data.ports[i]) {
                    int constr_index = (n.pu_type == PickupType::PICKUP ? (i - 1) : (np - 1 + i - 1));
                    port_coeff[constr_index]++;
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
    
    dummy = false;
}

void Column::operator=(const Column& other) {
    /*  prob is always the same */
    sol = other.sol;
    obj_coeff = other.obj_coeff;
    port_coeff = other.port_coeff;
    vc_coeff = other.vc_coeff;
    dummy = other.dummy;
}

void Column::make_dummy(const float huge_cost) {
    sol = Solution();
    obj_coeff = huge_cost;
    port_coeff = vector<float>(2 * (prob.data.num_ports - 1), 1);
    vc_coeff = vector<float>(prob.data.num_vessel_classes, 0);
    dummy = true;
}

bool Column::is_compatible_with_unite_rule(VisitRule vr) const {
    const Graph& g = prob.graphs.at(sol.vessel_class);
    
    for(const Edge& e : sol.path) {
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
    const Graph& g = prob.graphs.at(sol.vessel_class);
    
    for(const Edge& e : sol.path) {
        Node orig = *g.graph[source(e, g.graph)];
        Node dest = *g.graph[target(e, g.graph)];
        
        /*  If orig~vr.first and dest~vr.second then the path is not compatible! */
        if(orig.same_row_as(*vr.first) && dest.same_row_as(*vr.second)) {
            return false;
        }
    }
    
    return true;
}

Column Column::transfer_to(const Problem& other_prob) const {
    const Graph& other_g = other_prob.graphs.at(sol.vessel_class);
    const Graph& my_g = prob.graphs.at(sol.vessel_class);
    
    Path other_p = other_g.transfer_path(sol.path, my_g);
    Solution other_s(other_p, sol.cost, sol.reduced_cost, sol.vessel_class);
    Column other_c(other_prob, other_s, obj_coeff, port_coeff, vc_coeff, dummy);
    
    return other_c;
}

ostream& operator<<(ostream& out, const Column& c) {
    cout << setw(6) << c.obj_coeff << " | ";
    int hs = c.port_coeff.size() / 2;
    for(int i = 0; i < hs; i++) {
        cout << c.port_coeff[i] << " " << c.port_coeff[hs + i] << "\t";
    }
    cout << "| ";
    for(int i = 0; i < c.vc_coeff.size(); i++) {
        cout << c.vc_coeff[i] << " ";
    }
}