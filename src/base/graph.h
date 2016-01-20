//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <set>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
using namespace boost;

#include <base/arc.h>
#include <base/graph_properties.h>
#include <base/node.h>
#include <base/vessel_class.h>

typedef adjacency_list<listS, listS, bidirectionalS, std::shared_ptr<Node>, std::shared_ptr<Arc>, GraphProperties> BGraph;

typedef graph_traits<BGraph>::vertex_iterator vit;
typedef graph_traits<BGraph>::edge_iterator eit;
typedef graph_traits<BGraph>::in_edge_iterator ieit;
typedef graph_traits<BGraph>::out_edge_iterator oeit;

typedef graph_traits<BGraph>::vertex_descriptor Vertex;
typedef graph_traits<BGraph>::edge_descriptor Edge;

typedef std::vector<Edge> Path;

typedef std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> VisitRule;
typedef std::vector<VisitRule> VisitRuleList;

using ErasedEdges = std::map<Vertex, std::set<Edge>>;

class Graph {
public:
    BGraph                              graph;
    std::shared_ptr<VesselClass>        vessel_class;
    std::string                         name;
    std::vector<std::shared_ptr<Arc>>   ordered_arcs;

    Graph() {}
    Graph(const BGraph& graph, std::shared_ptr<VesselClass> vessel_class, const std::string& name) : graph(graph), vessel_class(vessel_class), name(name) {}
    Graph(BGraph&& graph, std::shared_ptr<VesselClass> vessel_class, const std::string& name) : graph(graph), vessel_class(vessel_class), name(name) {}
    
    void print(bool detailed = false) const;
    void print_path(const Path& p, std::ostream& out = std::cerr) const;
    
    /*  Order the arcs by cost (the most expensive first) and puts them in ordered_arcs */
    void sort_arcs();
    
    /*  The first item is true if the H1/h2 vertex has been found or false otherwise
        The second item is the vertex (in case it has been found) */
    std::pair<bool, Vertex> h1() const;
    std::pair<bool, Vertex> h2() const;
    
    /*  Add boost vertex/edge indices and computes upper bounds on the number
        of pu/de ports that can be visited */
    void prepare_for_labelling();
    
    /*  Used in branching when we want to enforce that n2->port is [not] visited just
        after n1->port */
    ErasedEdges get_erased_edges_from_rules(ErasedEdges already_erased, const VisitRuleList& unite_rules, const VisitRuleList& separate_rules) const;
    
    /*  Remove edges to reduce the graphs */
    ErasedEdges reduce_graph(double percentage, ErasedEdges already_erased) const;
    ErasedEdges smart_reduce_graph(double min_chance, double max_chance, ErasedEdges already_erased) const;

    /*  The highest/lowest dual prize among all ports */
    double max_dual_prize() const;
    double min_dual_prize() const;

    /*  The first item is true if the vertex has been found or false otherwise
        The second item is the vertex (in case it has been found) */
    std::pair<bool, Vertex> get_vertex(const Port& p, PickupType pu, int t) const;
    
    /*  Calculates the cost of a path as the sum of the cost of the edges */
    double calculate_cost(const Path& p) const;
    
    /*  Gets the correct dual value */
    double dual_of(const Node& n) const;
    
    /* Produces a text dump of the graph */
    void dump() const;
    
private:
    std::pair<bool, Vertex> get_vertex_by_node_type(NodeType n_type) const;
};

#endif