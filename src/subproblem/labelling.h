//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef LABELLING_H
#define LABELLING_H

#include <memory>
#include <unordered_map>
#include <utility>

#include <boost/functional/hash.hpp>

#include <base/graph.h>
#include <base/vessel_class.h>
#include <subproblem/id_maps.h>

class Label {
public:
    const std::shared_ptr<const Graph>& g;
    int             q_pickupable;
    int             q_deliverable;
    double          cost;
    
    Label(const std::shared_ptr<const Graph>& g,
          int qp,
          int qd,
          double c = 0) : g(g), q_pickupable(qp), q_deliverable(qd), cost(c) {}
          
    bool operator==(const Label& other) const;
    bool operator<(const Label& other) const;
};

typedef std::unordered_map<
    std::pair<std::shared_ptr<Port>, PickupType>, bool,
    boost::hash<std::pair<std::shared_ptr<Port>,PickupType>>> VisitedPortsFlags;

class ElementaryLabel : public Label {
public:
    VisitedPortsFlags visited_ports;
    
    ElementaryLabel(const std::shared_ptr<const Graph>& g, int qp, int qd, double c, const VisitedPortsFlags& vp) : Label(g, qp, qd, c), visited_ports(vp) {}
    
    bool operator==(const ElementaryLabel& other) const;
    bool operator<(const ElementaryLabel& other) const;
};

class LabelExtender {
public:
    bool operator()(const BGraph& graph, Label& new_label, const Label& label, const Edge& e) const;
    bool operator()(const BGraph& graph, ElementaryLabel& new_label, const ElementaryLabel& label, const Edge& e) const;
};

class Dominance {
public:
    bool operator()(const Label& l1, const Label& l2) const { return (l1 == l2 || l1 < l2); }
    bool operator()(const ElementaryLabel& l1, const ElementaryLabel& l2) const { return (l1 == l2 || l1 < l2); }
};

#endif