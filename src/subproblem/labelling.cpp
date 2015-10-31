//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <limits>

#include <subproblem/labelling.h>

bool operator==(const ElementaryLabel& lhs, const ElementaryLabel& rhs) {
    return( lhs.del == rhs.del &&
            lhs.por == rhs.por &&
            std::abs(lhs.cost - rhs.cost) < Label::EPS);
}

bool operator<(const ElementaryLabel& lhs, const ElementaryLabel& rhs) {
    if(rhs.cost < lhs.cost - Label::EPS) { return false; }
    if(rhs.del > lhs.del) { return false; }
    for(const auto& vp : rhs.por) { if(std::find(lhs.por.begin(), lhs.por.end(), vp) == lhs.por.end()) { return false; } }
    return true;
}

bool operator==(const Label& lhs, const Label& rhs) {
    return( lhs.del == rhs.del &&
            lhs.pic == rhs.pic &&
            std::abs(lhs.cost - rhs.cost) < Label::EPS);
}

bool operator<(const Label& lhs, const Label& rhs) {
    if(rhs.cost < lhs.cost - Label::EPS) { return false; }
    if(rhs.del > lhs.del) { return false; }
    if(rhs.pic > lhs.pic) { return false; }
    return true;
}

bool LabelExtender::operator()(const BGraph& graph, ElementaryLabel& new_label, const ElementaryLabel& label, const Edge& e) const {
    auto arc = *graph[e];
    auto dest_node = *graph[target(e, graph)];
    auto dest_port = std::make_pair(dest_node.port, dest_node.pu_type);
    
    new_label.por = label.por;
    new_label.por.erase(std::remove(new_label.por.begin(), new_label.por.end(), dest_port), new_label.por.end());
    
    new_label.cost = label.cost + arc.cost - label.g->dual_of(dest_node) - dest_node.penalty();
    new_label.pic = label.pic - dest_node.pu_demand();
    new_label.del = std::min(label.pic- dest_node.pu_demand(), label.del - dest_node.de_demand());
    
    return( std::find(label.por.begin(), label.por.end(), dest_port) != label.por.end() &&
            label.pic >= dest_node.pu_demand() &&
            label.del >= dest_node.de_demand());
}

bool LabelExtender::operator()(const BGraph& graph, Label& new_label, const Label& label, const Edge& e) const {
    auto arc = *graph[e];
    auto dest_node = *graph[target(e, graph)];
    
    new_label.cost = label.cost + arc.cost - label.g->dual_of(dest_node) - dest_node.penalty();
    new_label.pic = label.pic - dest_node.pu_demand();
    new_label.del = std::min(label.pic- dest_node.pu_demand(), label.del - dest_node.de_demand());
    
    return (label.pic >= dest_node.pu_demand() && label.del >= dest_node.de_demand());
}