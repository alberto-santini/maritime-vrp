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

typedef std::unordered_map<std::shared_ptr<Port>, pair<float, float>> PortDuals;
typedef std::unordered_map<std::shared_ptr<VesselClass>, float> VcDuals;

class Graph {
public:
    BGraph                          graph;
    std::shared_ptr<VesselClass>    vessel_class;
    string                          name;
    PortDuals                       port_duals;
    float                           vc_dual;
    int                             pu_upper_bound;
    int                             de_upper_bound;

    Graph() {}
    Graph(const BGraph graph,
          std::shared_ptr<VesselClass> vessel_class,
          const string name,
          const PortDuals port_duals = PortDuals(),
          const float vc_dual = 0) : graph(graph), vessel_class(vessel_class), name(name), port_duals(port_duals), vc_dual(vc_dual) {}
    
    void print(const bool detailed = false) const;
    void print_path(const Path& p) const;
    
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
    
    /*  Gets the correct dual value */
    float dual_of(const Node n) const;
    
    /*  Transfers a path fro a subgraph to the current graph. Typically the subgraph
        is the reduced graph of the current graph. */
    Path transfer_path(const Path& path, const Graph& subgraph) const;
    
    int n_port_ub() const { return (pu_upper_bound + de_upper_bound); }
    
private:
    pair<bool, Vertex> get_vertex_by_node_type(const NodeType n_type) const;
};

#endif