//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef LABELLING_H
#define LABELLING_H

#include <memory>
#include <unordered_map>
#include <utility>
#include <iostream>

#include <boost/functional/hash.hpp>
#include <boost/graph/r_c_shortest_paths.hpp>

#include "../base/graph.h"
#include "../base/vessel_class.h"
#include "id_maps.h"

namespace mvrp {
    using VisitablePorts = std::vector<std::pair<std::shared_ptr<Port>, PortType>>;

    struct Label {
        std::shared_ptr<const Graph> g;
        int pic;
        int del;
        double cost;

        static constexpr double EPS = 0.001;

        Label(std::shared_ptr<const Graph> g, int p, int d, double c = 0) : g(g), pic(p), del(d), cost(c) {}
    };

    bool operator==(const Label &lhs, const Label &rhs);
    bool operator<(const Label &lhs, const Label &rhs);

    std::ostream &operator<<(std::ostream &out, const Label &l);

    struct ElementaryLabel : public Label {
        VisitablePorts por;
        ElementaryLabel(std::shared_ptr<const Graph> g, int p, int d, double c, const VisitablePorts &v) : Label(g, p, d, c), por(v) {}
    };

    bool operator==(const ElementaryLabel &lhs, const ElementaryLabel &rhs);
    bool operator<(const ElementaryLabel &lhs, const ElementaryLabel &rhs);

    struct LabelExtender {
        const ErasedEdges &erased;

        LabelExtender(const ErasedEdges &erased) : erased{erased} {}

        bool operator()(const BGraph &graph, Label &new_label, const Label &label, const Edge &e) const;
        bool operator()(const BGraph &graph, ElementaryLabel &new_label, const ElementaryLabel &label, const Edge &e) const;
    };

    struct Dominance {
        bool operator()(const Label &l1, const Label &l2) const { return l1 == l2 || l1 < l2; }
        bool operator()(const ElementaryLabel &l1, const ElementaryLabel &l2) const { return l1 == l2 || l1 < l2; }
    };
}

#endif