//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <subproblem/labelling.h>

bool Label::operator==(const Label& other) const {
    return( q_pickupable == other.q_pickupable &&
            q_deliverable == other.q_deliverable &&
            n_visited == other.n_visited &&
            fabs(cost - other.cost) < numeric_limits<float>::epsilon());
}

bool Label::operator<(const Label& other) const {
    bool strict = false;
        
    if( q_pickupable > other.q_pickupable ||
        q_deliverable > other.q_deliverable ||
        n_visited < other.n_visited ||
        cost < other.cost - numeric_limits<float>::epsilon()) {
            strict = true;
    }
    
    bool dom = ( q_pickupable >= other.q_pickupable &&
                 q_deliverable >= other.q_deliverable &&
                 n_visited <= other.n_visited &&
                 cost <= other.cost &&
                 strict);
    
    return dom;
}

bool LabelExtender::operator()(const BGraph& graph, Label& new_label, const Label& label, Edge e) const {
    Node n_dest = *graph[target(e, graph)];
    
    new_label.q_pickupable = label.q_pickupable - n_dest.pu_demand();
    new_label.q_deliverable = min(label.q_deliverable - n_dest.de_demand(), label.q_pickupable - n_dest.pu_demand());
    
    new_label.n_visited = label.n_visited + (n_dest.n_type == NodeType::REGULAR_PORT ? 1 : 0);
    
    float dual = (n_dest.n_type == NodeType::REGULAR_PORT ? (n_dest.pu_type == PickupType::PICKUP ? label.port_duals.at(n_dest.port).first : label.port_duals.at(n_dest.port).second) : -label.vc_dual);
    
    new_label.cost = label.cost + graph[e]->cost - dual;
    new_label.port_duals = label.port_duals;
    new_label.vc_dual = label.vc_dual;
    new_label.upper_bound = label.upper_bound;
    
    bool ext = ( label.q_pickupable >= n_dest.pu_demand() &&
                 label.q_deliverable >= n_dest.de_demand() &&
                 (n_dest.n_type == NodeType::H2 || label.n_visited < label.upper_bound));
    
    return ext;
}