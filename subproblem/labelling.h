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
    // std::shared_ptr<Node> cur_node;
    PortDuals&      port_duals;
    float           vc_dual;
    int             upper_bound;
    
    Label(const int qp,
          const int qd,
          const int n,
          const float c,
          // std::shared_ptr<Node> nd,
          PortDuals& port_duals,
          const float vc_dual,
          const int upper_bound) : q_pickupable(qp), q_deliverable(qd), n_visited(n), cost(c), /* cur_node(nd), */ port_duals(port_duals), vc_dual(vc_dual), upper_bound(upper_bound) {}
          
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

// class ShortestPathDebugVisitor : public default_r_c_shortest_paths_visitor {
// public:
//     void on_label_popped(const r_c_shortest_paths_label<BGraph, Label>& l, const BGraph& g) {
//         cout << "Current label to " << *l.cumulated_resource_consumption.cur_node << endl;
//     }
//     void on_label_not_dominated(const r_c_shortest_paths_label<BGraph, Label>& l, const BGraph& g) {
//         cout << "\tLabel not dominated" << endl;
//     }
//     void on_label_dominated(const r_c_shortest_paths_label<BGraph, Label>& l, const BGraph& g) {
//         cout << "\tLabel dominated" << endl;
//     }
//     void on_label_not_feasible(const r_c_shortest_paths_label<BGraph, Label>& nl, const BGraph& g) {
//         cout << "\t\tExtension to " << *nl.cumulated_resource_consumption.cur_node << " not feasible!" << endl;
//     }
//     void on_label_feasible(const r_c_shortest_paths_label<BGraph, Label>& nl, const BGraph& g) {
//         cout << "\t\tExtension to " << *nl.cumulated_resource_consumption.cur_node << " feasible!" << endl;
//     }
// };

#endif