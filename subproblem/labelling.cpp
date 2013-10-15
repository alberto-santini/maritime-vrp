//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <subproblem/labelling.h>

bool Label::operator==(const Label& other) const {
    return( q_pickupable == other.q_pickupable &&
            q_deliverable == other.q_deliverable &&
            fabs(cost - other.cost) < numeric_limits<float>::epsilon());
}

bool Label::operator<(const Label& other) const {
    bool strict = false;
        
    if( q_pickupable > other.q_pickupable ||
        q_deliverable > other.q_deliverable ||
        cost < other.cost - numeric_limits<float>::epsilon()) {
            strict = true;
    }
    
    bool dom = ( q_pickupable >= other.q_pickupable &&
                 q_deliverable >= other.q_deliverable &&
                 cost <= other.cost &&
                 strict);
    
    return dom;
}

bool LabelExtender::operator()(const BGraph& graph, Label& new_label, const Label& label, Edge e) const {
    Node n_dest = *graph[target(e, graph)];
    
    new_label.q_pickupable = label.q_pickupable - n_dest.pu_demand();
    new_label.q_deliverable = min(label.q_deliverable - n_dest.de_demand(), label.q_pickupable - n_dest.pu_demand());
        
    float dual = (n_dest.n_type == NodeType::REGULAR_PORT ? (n_dest.pu_type == PickupType::PICKUP ? graph[graph_bundle].port_duals.at(n_dest.port).first : graph[graph_bundle].port_duals.at(n_dest.port).second) : graph[graph_bundle].vc_dual);
    
    new_label.cost = label.cost + graph[e]->cost - dual;
        
    bool ext = ( label.q_pickupable >= n_dest.pu_demand() &&
                 label.q_deliverable >= n_dest.de_demand());
    
    return ext;
}