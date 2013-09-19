//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <base/graph.h>

void Graph::print(const bool detailed) const {
    cout << "Graph: " << name << endl;
    cout << "Number of vertices: " << num_vertices(graph) << endl;
    cout << "Number of edges: " << num_edges(graph) << endl;
    
    if(detailed) {
        cout << "Vertices:" << endl;
        pair<vit, vit> vp;
        for(vp = vertices(graph); vp.first != vp.second; ++vp.first) {
            cout << *graph[*vp.first] << " - Prize: " << graph[*vp.first]->dual_prize << endl;
        }
        cout << "Edges:" << endl;
        pair<eit, eit> ep;
        for(ep = edges(graph); ep.first != ep.second; ++ep.first) {
            cout << *graph[source(*ep.first, graph)] << " -> " << *graph[target(*ep.first, graph)];
            cout << " - Cost: " << graph[*ep.first]->cost << endl;
        }
    }
}

pair<bool, Vertex> Graph::h1() const {
    return get_vertex_by_node_type(NodeType::H1);
}

pair<bool, Vertex> Graph::h2() const {
    return get_vertex_by_node_type(NodeType::H2);
}

pair<bool, Vertex> Graph::get_vertex_by_node_type(const NodeType n_type) const {
    pair<vit, vit> vp;
    for(vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        if(graph[*vp.first]->n_type == n_type) {
            return make_pair(true, *vp.first);
        }
    }
    return make_pair(false, Vertex());
}

void Graph::prepare_for_labelling() {
    int i = 0;
    vector<pair<std::shared_ptr<Port>, PickupType>> checked_ports;
    vector<int> pickup_demands;
    vector<int> delivery_demands;
    
    pair<vit, vit> vp;
    for(vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        Vertex v = *vp.first;
        graph[v]->boost_vertex_id = i++;
        if(graph[v]->n_type == NodeType::REGULAR_PORT) {
            auto pp = make_pair(graph[v]->port, graph[v]->pu_type);
            if(find(checked_ports.begin(), checked_ports.end(), pp) == checked_ports.end()) {
                checked_ports.push_back(pp);
                if(graph[v]->pu_type == PickupType::PICKUP) {
                    pickup_demands.push_back(graph[v]->port->pickup_demand);
                } else {
                    delivery_demands.push_back(graph[v]->port->delivery_demand);
                }
            }
        }
    }
    
    pu_upper_bound = Knapsack::solve(pickup_demands, vessel_class->capacity);
    de_upper_bound = Knapsack::solve(delivery_demands, vessel_class->capacity);
    
    i = 0;
    pair<eit, eit> ep;
    for(ep = edges(graph); ep.first != ep.second; ++ep.first) {
        graph[*ep.first]->boost_edge_id = i++;
    }
}

void Graph::unite_ports(VisitRule vr, Graph& dest) const {
    std::shared_ptr<Node> n1, n2;
    tie(n1, n2) = vr;
    
    if(n1->n_type != NodeType::REGULAR_PORT || n2->n_type != NodeType::REGULAR_PORT) {
        throw runtime_error("Trying to unite ports of nodes that are not both regular nodes");
    }
    
    if(n1->vessel_class != vessel_class || n2->vessel_class != vessel_class) {
        throw runtime_error("Trying to unite ports of nodes not both in this graph");
    }
    
    string new_name = name + " uniting " + n1->port->name + " with " + n2->port->name;
    dest = Graph(graph, vessel_class, new_name, dual_price);
    
    pair<vit, vit> vp;
    for(vp = vertices(dest.graph); vp.first != vp.second; ++vp.first) {
        Vertex v1 = *vp.first;
        
        if(dest.graph[v1]->same_row_as(*n1)) {
            oeit ei, ei_end, ei_next;
            tie(ei, ei_end) = out_edges(v1, dest.graph);
            for(ei_next = ei; ei != ei_end; ei = ei_next) {
                ++ei_next;
                Vertex v2 = target(*ei, dest.graph);
                if(!dest.graph[v2]->same_row_as(*n2)) {
                    remove_edge(*ei, dest.graph);
                }
            }
        } else if(dest.graph[v1]->same_row_as(*n2)) {
            ieit ei, ei_end, ei_next;
            tie(ei, ei_end) = in_edges(v1, dest.graph);
            for(ei_next = ei; ei != ei_end; ei = ei_next) {
                ++ei_next;
                Vertex v2 = source(*ei, dest.graph);
                if(!dest.graph[v2]->same_row_as(*n1)) {
                    remove_edge(*ei, dest.graph);
                }
            }
        }
    }
}

void Graph::separate_ports(VisitRule vr, Graph& dest) const {
    std::shared_ptr<Node> n1, n2;
    tie(n1, n2) = vr;
    
    if(n1->n_type != NodeType::REGULAR_PORT || n2->n_type != NodeType::REGULAR_PORT) {
        throw runtime_error("Trying to unite ports of nodes that are not both regular nodes");
    }
    
    if(n1->vessel_class != vessel_class || n2->vessel_class != vessel_class) {
        throw runtime_error("Trying to unite ports of nodes not both in this graph");
    }
    
    string new_name = name + " separating " + n1->port->name + " with " + n2->port->name;
    dest = Graph(graph, vessel_class, new_name, dual_price);

    pair<vit, vit> vp;
    for(vp = vertices(dest.graph); vp.first != vp.second; ++vp.first) {
        Vertex v1 = *vp.first;
        
        if(dest.graph[v1]->same_row_as(*n1)) {
            oeit ei, ei_end, ei_next;
            tie(ei, ei_end) = out_edges(v1, dest.graph);
            for(ei_next = ei; ei != ei_end; ei = ei_next) {
                ++ei_next;
                Vertex v2 = target(*ei, dest.graph);
                if(dest.graph[v2]->same_row_as(*n2)) {
                    remove_edge(*ei, dest.graph);
                }
            }
        }
    }
}

void Graph::reduce_graph(const float lambda, Graph& dest) const {
    float cost_limit = lambda * max_dual_prize();
    string new_name = name + " reduced for arc cost < " + to_string(cost_limit);
    dest = Graph(graph, vessel_class, new_name, dual_price);
    
    eit ei, ei_end, ei_next;
    tie(ei, ei_end) = edges(dest.graph);
    for(ei_next = ei; ei != ei_end; ei = ei_next) {
        ++ei_next;
        if(dest.graph[*ei]->cost >= cost_limit + numeric_limits<float>::epsilon()) {
            remove_edge(*ei, dest.graph);
        }
    }
}

float Graph::max_dual_prize() const {
    float max_prize = 0;
    
    pair<vit, vit> vp;
    for(vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        max_prize = max(max_prize, graph[*vp.first]->dual_prize);
    }
    
    return max_prize;
}

pair<bool, Vertex> Graph::get_vertex(std::shared_ptr<Port> p, const PickupType pu, const int t) const {
    pair<vit, vit> vp;
    for(vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        Node n = *graph[*vp.first];
        if(n.port == p && n.pu_type == pu && n.time_step == t) {
            return make_pair(true, *vp.first);
        }
    }
    
    return make_pair(false, Vertex());
}

float Graph::calculate_cost(const Path& p) const {
    float cost = 0;
    
    for(Edge e : p) {
        cost += graph[e]->cost;
    }
    
    return cost;
}