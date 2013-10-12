//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <column/column.h>

Column::Column(const std::shared_ptr<const Problem> prob, const Solution sol, const string created_by) : prob(prob), created_by(created_by) {
    const Graph& g = prob->graphs.at(sol.vessel_class);
    const Graph& node_g = sol.g;
    
    Path global_p = g.transfer_path(sol.path, node_g);
    Solution global_sol(global_p, sol.cost, sol.reduced_cost, sol.vessel_class, g);
    
    obj_coeff = global_sol.cost;    
    
    int np = prob->data.num_ports;
    int nv = prob->data.num_vessel_classes;
    
    /*  "np - 1" to remove the hub;
        "2 *" to create one coefficient for (port, pu) and one for (port, de) */
    port_coeff = vector<float>(2 * (np - 1), 0);
    for(const Edge& e : sol.path) {
        Node n = *g.graph[target(e, g.graph)];
        if(n.n_type == NodeType::REGULAR_PORT) {
            for(int i = 1; i < np; i++) {
                if(n.port == prob->data.ports[i]) {
                    int constr_index = (n.pu_type == PickupType::PICKUP ? (i - 1) : (np - 1 + i - 1));
                    port_coeff[constr_index]++;
                }
            }
        }
    }
    
    vc_coeff = vector<float>(nv, 0);
    for(int i = 0; i < nv; i++) {
        if(global_sol.vessel_class == prob->data.vessel_classes[i]) {
            vc_coeff[i] = 1;
        }
    }
    
    this->sol = global_sol;
    dummy = false;
}

void Column::make_dummy(const float huge_cost) {
    sol = Solution();
    obj_coeff = huge_cost;
    port_coeff = vector<float>(2 * (prob->data.num_ports - 1), 1);
    vc_coeff = vector<float>(prob->data.num_vessel_classes, 0);
    dummy = true;
    created_by = "dummy";
}

bool Column::is_compatible_with_unite_rule(VisitRule vr) const {
    if(dummy) {
        return true;
    }
    
    const Graph& g = sol.g;
    
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
    if(dummy) {
        return true;
    }
    
    const Graph& g = sol.g;
    
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

bool Column::has_cycles() const {
    for(const float& coeff : port_coeff) {
        if(coeff > 1 + numeric_limits<float>::epsilon()) {
            return true;
        }
    }
    return false;
}

ostream& operator<<(ostream& out, const Column& c) {
    out << setw(6) << c.obj_coeff << " | ";
    int hs = c.port_coeff.size() / 2;
    for(int i = 0; i < hs; i++) {
        out << c.port_coeff[i] << " " << c.port_coeff[hs + i] << "\t";
    }
    out << "| ";
    for(int i = 0; i < c.vc_coeff.size(); i++) {
        out << c.vc_coeff[i] << " ";
    }
    out << "by " << c.created_by << " ";
    return out;
}