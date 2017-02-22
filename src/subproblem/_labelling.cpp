//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include "_labelling.h"

namespace mvrp {

    bool operator==(const Label& lhs, const Label& rhs) {
        return( lhs.del == rhs.del &&
                lhs.pic == rhs.pic &&
                std::abs(lhs.cost - rhs.cost) < Label::EPS);
    }

    bool operator!=(const Label& lhs, const Label& rhs) {
        return !(lhs == rhs);
    }

    bool operator<=(const Label& lhs, const Label& rhs) {
        if(rhs.cost < lhs.cost - Label::EPS) { return false; }
        if(rhs.del > lhs.del) { return false; }
        if(rhs.pic > lhs.pic) { return false; }
        return true;
    }

    bool operator<(const Label& lhs, const Label& rhs) {
        return lhs <= rhs && lhs != rhs;
    }

    std::ostream& operator<<(std::ostream& out, const Label& l) {
        out << "(cost: " << l.cost << ", pic: " << l.pic << ", del: " << l.del << ")";
        return out;
    }

    bool operator==(const ElementaryLabel& lhs, const ElementaryLabel& rhs) {
        return( lhs.del == rhs.del &&
                lhs.por == rhs.por &&
                std::abs(lhs.cost - rhs.cost) < Label::EPS);
    }

    bool operator!=(const ElementaryLabel& lhs, const ElementaryLabel& rhs) {
        return !(lhs == rhs);
    }

    bool operator<=(const ElementaryLabel& lhs, const ElementaryLabel& rhs) {
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
    
        return true;
    }

    bool operator<(const ElementaryLabel& lhs, const ElementaryLabel& rhs) {
        return lhs <= rhs && lhs != rhs;
    }

    std::ostream& operator<<(std::ostream& out, const ElementaryLabel& l) {
        out << "(cost: " << l.cost << ", pic: " << l.pic << ", del: " << l.del << ", visitable: " << l.por.size() << " ports)";
        return out;
    }

    boost::optional<Label> LabelExtender::operator()(const BGraph& graph, const Label& label, const Edge& e) const {
        const Arc& arc = *graph[e];
    
        const Vertex& src_vertex = source(e, graph);
        const Vertex& trg_vertex = target(e, graph);
    
        const Node& trg_node = *graph[trg_vertex];
    
        // Erased edge, sorry!
        if(erased.find(src_vertex) != erased.end() && erased.at(src_vertex).find(e) != erased.at(src_vertex).end()) { return boost::none; }
    
        Label new_label = label;
        
        // Not enough pickup "space", sorry!
        if(label.pic < trg_node.pu_demand()) { return boost::none; }
    
        new_label.pic = label.pic - trg_node.pu_demand();
    
        // Not enough delivery "space", sorry!
        if(label.del < trg_node.de_demand()) { return boost::none; }
    
        new_label.del = std::min(label.pic - trg_node.pu_demand(), label.del - trg_node.de_demand());
    
        new_label.cost = label.cost + arc.cost - label.g.dual_of(trg_node) - trg_node.penalty();
    
        return new_label;
    }

    boost::optional<ElementaryLabel> LabelExtender::operator()(const BGraph& graph, const ElementaryLabel& label, const Edge& e) const {
        const Arc& arc = *graph[e];
    
        const Vertex& src_vertex = source(e, graph);
        const Vertex& trg_vertex = target(e, graph);
    
        const Node& trg_node = *graph[trg_vertex];
    
        auto dest_port = std::make_pair(trg_node.port, trg_node.pu_type);
    
        // Erased edge, sorry!
        if(erased.find(src_vertex) != erased.end() && erased.at(src_vertex).find(e) != erased.at(src_vertex).end()) { return boost::none; }
    
        // Target port not visitable, sorry!
        if(std::find(label.por.begin(), label.por.end(), dest_port) == label.por.end()) { return boost::none; }
    
        ElementaryLabel new_label = label;
    
        new_label.por = label.por;
        new_label.por.erase(std::remove(new_label.por.begin(), new_label.por.end(), dest_port), new_label.por.end());
    
        // Not enough pickup "space", sorry!
        if(label.pic < trg_node.pu_demand()) { return boost::none; }
    
        new_label.pic = label.pic - trg_node.pu_demand();
    
        // Not enough delivery "space", sorry!
        if(label.del < trg_node.de_demand()) { return boost::none; }
    
        new_label.del = std::min(label.pic - trg_node.pu_demand(), label.del - trg_node.de_demand());
    
        new_label.cost = label.cost + arc.cost - label.g.dual_of(trg_node) - trg_node.penalty();
    
        return new_label;
    }
}