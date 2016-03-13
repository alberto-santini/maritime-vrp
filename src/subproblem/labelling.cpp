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
    if(rhs.pic > lhs.pic) { return false; }
    
    // If there is any port visitable by RHS...
    if(std::any_of(
        rhs.por.begin(),
        rhs.por.end(),
        [&] (const auto& vp) {
            // ...that is not visitable by LHS
            return std::find(lhs.por.begin(), lhs.por.end(), vp) == lhs.por.end();
        }
    )) { return false; }
    
    return !(lhs == rhs);
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
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& out, const Label& l) {
    out << "(cost: " << l.cost << ", pic: " << l.pic << ", del: " << l.del << ")";
    return out;
}

bool LabelExtender::operator()(const BGraph& graph, ElementaryLabel& new_label, const ElementaryLabel& label, const Edge& e) const {
    const Arc& arc = *graph[e];
    const Vertex& src_vertex = source(e, graph);
    const Node& dest_node = *graph[target(e, graph)];
    auto dest_port = std::make_pair(dest_node.port, dest_node.pu_type);
    
    if(erased.find(src_vertex) != erased.end() && erased.at(src_vertex).find(e) != erased.at(src_vertex).end()) { return false; }
    if(std::find(label.por.begin(), label.por.end(), dest_port) == label.por.end()) { return false; }
    
    new_label.por = label.por;
    new_label.por.erase(std::remove(new_label.por.begin(), new_label.por.end(), dest_port), new_label.por.end());
    
    if(label.pic < trg_node.pu_demand()) { return false; }
    new_label.pic = label.pic - dest_node.pu_demand();
    
    if(label.del < trg_node.de_demand()) { return false; }
    new_label.del = std::min(label.pic- dest_node.pu_demand(), label.del - dest_node.de_demand());
    
    new_label.cost = label.cost + arc.cost - label.g->dual_of(dest_node) - dest_node.penalty();
    
    return true;
}

bool LabelExtender::operator()(const BGraph& graph, Label& new_label, const Label& label, const Edge& e) const {
    const Arc& arc = *graph[e];
    const Vertex& src_vertex = source(e, graph);
    const Node& dest_node = *graph[target(e, graph)];
    
    if(erased.find(src_vertex) != erased.end() && erased.at(src_vertex).find(e) != erased.at(src_vertex).end()) { return false; }
    
    if(label.pic < trg_node.pu_demand()) { return false; }
    new_label.pic = label.pic - dest_node.pu_demand();
    
    if(label.del < trg_node.de_demand()) { return false; }
    new_label.del = std::min(label.pic- dest_node.pu_demand(), label.del - dest_node.de_demand());
    
    new_label.cost = label.cost + arc.cost - label.g->dual_of(dest_node) - dest_node.penalty();
    
    return true;
}