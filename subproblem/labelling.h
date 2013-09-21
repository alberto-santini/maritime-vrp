//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef LABELLING_H
#define LABELLING_H

#include <base/base.h>
#include <base/graph.h>
#include <base/vessel_class.h>
#include <subproblem/id_maps.h>

class Label {
public:
    int             q_pickupable;
    int             q_deliverable;
    int             n_visited;
    float           cost;
    PortDuals&      port_duals;
    float           vc_dual;
    int             upper_bound;
    
    Label(const int qp,
          const int qd,
          const int n,
          const float c,
          PortDuals& port_duals,
          const float vc_dual,
          const int upper_bound) : q_pickupable(qp), q_deliverable(qd), n_visited(n), cost(c), port_duals(port_duals), vc_dual(vc_dual), upper_bound(upper_bound) {}
          
    bool operator==(const Label& other) const;
    bool operator<(const Label& other) const;
};

class LabelExtender {
public:
    bool operator()(const BGraph& graph, Label& new_label, const Label& label, Edge e) const;
};

class Dominance {
public:
    bool operator()(const Label& l1, const Label& l2) const { return (l1 == l2 || l1 < l2); }
};

#endif