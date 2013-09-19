//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef GRAPH_H
#define GRAPH_H

#include <base/base.h>
#include <base/node.h>
#include <base/arc.h>
#include <base/port.h>
#include <base/vessel_class.h>
#include <util/knapsack.h>

typedef adjacency_list<listS, listS, bidirectionalS, std::shared_ptr<Node>, std::shared_ptr<Arc>> BGraph;

typedef graph_traits<BGraph>::vertex_iterator vit;
typedef graph_traits<BGraph>::edge_iterator eit;
typedef graph_traits<BGraph>::in_edge_iterator ieit;
typedef graph_traits<BGraph>::out_edge_iterator oeit;

typedef graph_traits<BGraph>::vertex_descriptor Vertex;
typedef graph_traits<BGraph>::edge_descriptor Edge;

typedef vector<Edge> Path;

typedef pair<std::shared_ptr<Node>, std::shared_ptr<Node>> VisitRule;
typedef vector<pair<std::shared_ptr<Node>, std::shared_ptr<Node>>> VisitRuleList;

class Graph {
public:
    BGraph                          graph;
    std::shared_ptr<VesselClass>    vessel_class;
    string                          name;
    float                           dual_price;
    int                             pu_upper_bound;
    int                             de_upper_bound;
    
    Graph(const BGraph graph = BGraph(),
          std::shared_ptr<VesselClass> vessel_class = nullptr,
          const string name = "Graph",
          const float dual_price = 0) : graph(graph), vessel_class(vessel_class), name(name), dual_price(dual_price) {
        pu_upper_bound = 0;
        de_upper_bound = 0;
    }
    
    void print(const bool detailed = false) const;
    
    /*  The first item is true if the H1/h2 vertex has been found or false otherwise
        The second item is the vertex (in case it has been found) */
    pair<bool, Vertex> h1() const;
    pair<bool, Vertex> h2() const;
    
    /*  Add boost vertex/edge indices and computes upper bounds on the number
        of pu/de ports that can be visited */
    void prepare_for_labelling();
    
    /*  Used in branching when we want to enforce that n2->port is [not] visited just
        after n1->port */
    void unite_ports(VisitRule vr, Graph& dest) const;
    void separate_ports(VisitRule vr, Graph& dest) const;
    
    /*  Creates the reduced graphs, where edges with high cost are removed */
    void reduce_graph(const float lambda, Graph& dest) const;

    /*  The highest dual prize among all ports */
    float max_dual_prize() const;

    /*  The first item is true if the vertex has been found or false otherwise
        The second item is the vertex (in case it has been found) */
    pair<bool, Vertex> get_vertex(std::shared_ptr<Port> p, const PickupType pu, const int t) const;
    
    /*  Calculates the cost of a path as the sum of the cost of the edges */
    float calculate_cost(const Path& p) const;
    
private:
    pair<bool, Vertex> get_vertex_by_node_type(const NodeType n_type) const;
};

#endif