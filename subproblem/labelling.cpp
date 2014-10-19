//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <limits>

#include <subproblem/labelling.h>

bool Label::operator==(const Label& other) const {
    return( q_pickupable == other.q_pickupable &&
            q_deliverable == other.q_deliverable &&
            fabs(cost - other.cost) < std::numeric_limits<float>::epsilon());
}

bool Label::operator<(const Label& other) const {
    bool strict = false;
        
    if( q_pickupable > other.q_pickupable ||
        q_deliverable > other.q_deliverable ||
        cost < other.cost - std::numeric_limits<float>::epsilon()) {
            strict = true;
    }
    
    bool dom = ( q_pickupable >= other.q_pickupable &&
                 q_deliverable >= other.q_deliverable &&
                 cost <= other.cost &&
                 strict);
    
    return dom;
}

bool ElementaryLabel::operator==(const ElementaryLabel& other) const {
    return (Label::operator==(other) && visited_ports == other.visited_ports);
}

bool ElementaryLabel::operator<(const ElementaryLabel& other) const {
    auto strict = false;
    auto visited_d = true;
        
    if( q_pickupable > other.q_pickupable ||
        q_deliverable > other.q_deliverable ||
        cost < other.cost - std::numeric_limits<float>::epsilon()) {
            strict = true;
    }
    
    for(const auto& pp : visited_ports) {
        if((pp.second != other.visited_ports.at(pp.first)) && pp.second) {
            visited_d = false;
        }
    }
    
    strict = strict || (visited_d && (visited_ports != other.visited_ports));
    
    auto dom = ( q_pickupable >= other.q_pickupable &&
                 q_deliverable >= other.q_deliverable &&
                 cost <= other.cost &&
                 visited_d &&
                 strict);
    
    return dom;
}

bool LabelExtender::operator()(const BGraph& graph, Label& new_label, const Label& label, const Edge& e) const {
    auto n_dest = *graph[target(e, graph)];
    
    new_label.q_pickupable = label.q_pickupable - n_dest.pu_demand();
    new_label.q_deliverable = std::min(label.q_deliverable - n_dest.de_demand(), label.q_pickupable - n_dest.pu_demand());
        
    auto dual = (n_dest.n_type == NodeType::REGULAR_PORT ? (n_dest.pu_type == PickupType::PICKUP ? graph[graph_bundle].port_duals.at(n_dest.port).first : graph[graph_bundle].port_duals.at(n_dest.port).second) : graph[graph_bundle].vc_dual);
    
    new_label.cost = label.cost + graph[e]->cost - dual;
        
    auto ext = ( label.q_pickupable >= n_dest.pu_demand() &&
                 label.q_deliverable >= n_dest.de_demand());
    
    return ext;
}

bool LabelExtender::operator()(const BGraph& graph, ElementaryLabel& new_label, const ElementaryLabel& label, const Edge& e) const {
    auto n_dest = *graph[target(e, graph)];
    
    new_label.q_pickupable = label.q_pickupable - n_dest.pu_demand();
    new_label.q_deliverable = std::min(label.q_deliverable - n_dest.de_demand(), label.q_pickupable - n_dest.pu_demand());
        
    auto dual = (n_dest.n_type == NodeType::REGULAR_PORT ? (n_dest.pu_type == PickupType::PICKUP ? graph[graph_bundle].port_duals.at(n_dest.port).first : graph[graph_bundle].port_duals.at(n_dest.port).second) : graph[graph_bundle].vc_dual);
    
    new_label.cost = label.cost + graph[e]->cost - dual;
    
    auto dest_pp = std::make_pair(n_dest.port, n_dest.pu_type);
    new_label.visited_ports[dest_pp] = true;
        
    auto ext = ( label.q_pickupable >= n_dest.pu_demand() &&
                 label.q_deliverable >= n_dest.de_demand() &&
                 !label.visited_ports.at(dest_pp));
    
    return ext;
}