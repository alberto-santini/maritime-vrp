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

using VisitablePorts = std::vector<std::pair<std::shared_ptr<Port>, PickupType>>;

class Label {
public:
    std::shared_ptr<const Graph> g;
    int pic;
    int del;
    double cost;
    
    static constexpr double EPS = 0.00001;
    
    Label(std::shared_ptr<const Graph> g, int p, int d, double c = 0) : g(g), pic(p), del(d), cost(c) {}
};

bool operator==(const Label& lhs, const Label& rhs);
bool operator<(const Label& lhs, const Label& rhs);

class ElementaryLabel : public Label {
public:
    VisitablePorts por;
    
    ElementaryLabel(std::shared_ptr<const Graph> g, int p, int d, double c, const VisitablePorts& v) : Label(g, p, d, c), por(v) {}
};

bool operator==(const ElementaryLabel& lhs, const ElementaryLabel& rhs);
bool operator<(const ElementaryLabel& lhs, const ElementaryLabel& rhs);

class LabelExtender {
public:
    bool operator()(const BGraph& graph, Label& new_label, const Label& label, const Edge& e) const;
    bool operator()(const BGraph& graph, ElementaryLabel& new_label, const ElementaryLabel& label, const Edge& e) const;
};

class Dominance {
public:
    bool operator()(const Label& l1, const Label& l2) const { return l1 < l2 || l1 == l2; }
    bool operator()(const ElementaryLabel& l1, const ElementaryLabel& l2) const { return l1 < l2 || l1 == l2; }
};

#endif