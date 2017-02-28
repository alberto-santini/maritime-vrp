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

#include "arc.h"
#include "graph_properties.h"
#include "node.h"
#include "vessel_class.h"

namespace mvrp {
    using BGraph = boost::adjacency_list<boost::listS, boost::listS, boost::bidirectionalS, std::shared_ptr<Node>, std::shared_ptr<Arc>, GraphProperties>;

    using vit = boost::graph_traits<BGraph>::vertex_iterator;
    using eit = boost::graph_traits<BGraph>::edge_iterator;
    using ieit = boost::graph_traits<BGraph>::in_edge_iterator;
    using oeit = boost::graph_traits<BGraph>::out_edge_iterator;

    using Vertex = boost::graph_traits<BGraph>::vertex_descriptor;
    using Edge = boost::graph_traits<BGraph>::edge_descriptor;

    using Path = std::vector<Edge>;
    using ErasedEdges = std::map<Vertex, std::set<Edge>>;

    struct Graph {
        /**
         * Underlying boost graph.
         */
        BGraph graph;

        /**
         * Vessel class associated with the graph.
         */
        std::shared_ptr<VesselClass> vessel_class;

        /**
         * Vector used to keep an ordering of the arcs, e.g. by their cost.
         */
        std::vector<std::shared_ptr<Arc>> ordered_arcs;

        Graph() {}

        Graph(const BGraph& graph, std::shared_ptr<VesselClass> vessel_class) :
            graph(graph), vessel_class(vessel_class) {}

        Graph(BGraph&& graph, std::shared_ptr<VesselClass> vessel_class) :
            graph(graph), vessel_class(vessel_class) {}

        /**
         * Prints basic information about the graph.
         * @param detailed  If true, will also print the list of vertices and edges
         */
        void print(bool detailed = false) const;

        /**
         * Prints a path in the graph.
         * @param p     The path
         * @param out   The out-stream where to print
         */
        void print_path(const Path &p, std::ostream &out = std::cerr) const;

        /**
         * Orders the arcs by cost (the most expensive first) and puts them in ordered_arcs
         */
        void sort_arcs();

        /**
         * Finds the source vertex.
         * @return  A pair where the first element is true iff the source vertex has been found
         *          and, in this case, the second element is the required vertex descriptor.
         */
        std::pair<bool, Vertex> get_source_vertex() const;

        /**
          * Finds the sink vertex.
          * @return  A pair where the first element is true iff the sink vertex has been found
          *          and, in this case, the second element is the required vertex descriptor.
          */
        std::pair<bool, Vertex> get_sink_vertex() const;

        /**
         * Finds the vertex corresponding to a certain port, with a certain type, at
         * a certain time.
         * @param p     The port
         * @param pu    The type (pickup/delivery)
         * @param t     The time instant
         * @return      A pair where the first element is true iff the vertex has been found
         *              and, in this case, the second element is the required vertex descriptor.
         */
        std::pair<bool, Vertex> get_vertex(const Port& p, PortType pu, int t) const;

        /**
         * Prepares the graph for labelling: adds unique consecutive ids to vertices and edges
         * and computes upper bounds on the number of pickup and delivery ports that the associated
         * vessel can visit.
         */
        void prepare_for_labelling();

        /*  Used in branching when we want to enforce that n2->port is [not] visited just
            after n1->port */

//        /**
//         * Gives a list of edges that need to be removed, in order for the subproblem to be
//         * compliant with new branching rules.
//         * @param already_erased    A list of edges already removed.
//         * @param unite_rules       A "unite" branching rule.
//         * @param separate_rules    A "separate" branching rule.
//         * @return                  A list of edges to remove (including those given in input).
//         */
//        ErasedEdges get_erased_edges_from_rules(ErasedEdges already_erased, const VisitRuleList& unite_rules,
//                                                const VisitRuleList& separate_rules) const;

        /**
         * Gives a list of edges that need to be removed in order to reduce the graph. In particular,
         * it will remove the ``ratio'' (between 0 and 1) of remaining edges with the highest cost.
         * By remaining edges we mean those that are not in ``already_erased''. For example, if half
         * of the graph edges are in ``already_erased'' and ``ratio'' is 0.5, this method will give
         * a list containing 75% of the edges: those that were in ``already_erased'' plus the most
         * expensive half of the other edges.
         * @param ratio             Share of remaining edges to remove
         * @param already_erased    List of edges which have already been removed
         * @return                  List of all edges to remove
         */
        ErasedEdges reduce_graph(double ratio, ErasedEdges already_erased) const;

        /**
         * Similar to ``reduce_graph'', but each arc has a certain probability of being removed. The
         * cheapest arc will have a probability of ``min_chance'' to be removed; the most expensive
         * arc will have a probability of ``max_chance''. All other arcs will have probabilities
         * linearly distributed between these two extremes, based on their cost.
         * @param min_chance        Probability that the cheapest arc is removed
         * @param max_chance        Probability that the most expensive arc is removed
         * @param already_erased    List of edges which have already been removed
         * @return                  List of all edges to remove
         */
        ErasedEdges smart_reduce_graph(double min_chance, double max_chance, ErasedEdges already_erased) const;

        /**
         * Gives the maximum dual prize that can be collected at any port.
         */
        double max_dual_prize() const;

        /**
         * Gives the minimum dual prize that can be collected at any port.
         */
        double min_dual_prize() const;

        /**
         * Gives the cost of a path (as the sum of the cost of its edges).
         * @param p The path
         * @return  The path's cost
         */
        double calculate_path_cost(const Path& p) const;

        /**
         * Gets the dual prize associated with a certain node.
         * @param n The node
         * @return  The dual prize
         */
        double dual_of(const Node &n) const;

        /**
         * Produces a text dump of the graph
         */
        void dump() const;

    private:
        std::pair<bool, Vertex> get_vertex_by_node_type(NodeType n_type) const;
    };
}

#endif