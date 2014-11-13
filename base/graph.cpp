//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>
#include <limits>
#include <stdexcept>

#include <base/graph.h>
#include <util/knapsack.h>

void Graph::print(bool detailed) const {
    std::cout << "Graph: " << name << std::endl;
    std::cout << "Number of vertices: " << num_vertices(graph) << std::endl;
    std::cout << "Number of edges: " << num_edges(graph) << std::endl;
    
    if(detailed) {
        std::cout << "Vertices:" << std::endl;
        for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
            auto n = *graph[*vp.first];
            std::string prize;
            try {
                prize = std::to_string(dual_of(n));
            } catch(std::out_of_range& e) {
                prize = "undefined";
            }
            std::cout << n << " - Dual prize/price: " << prize << std::endl;
        }
        std::cout << "Edges:" << std::endl;
        for(auto ep = edges(graph); ep.first != ep.second; ++ep.first) {
            std::cout << *graph[source(*ep.first, graph)] << " -> " << *graph[target(*ep.first, graph)];
            std::cout << " - Cost: " << graph[*ep.first]->cost << std::endl;
        }
    }
}

void Graph::print_path(const Path& p, std::ostream& out) const {
    for(auto pit = p.rbegin(); pit != p.rend(); ++pit) {
        out << *graph[source(*pit, graph)] << " -> " << *graph[target(*pit, graph)] << std::endl;
    }
}

void Graph::sort_arcs() {
    if(ordered_arcs.size() != num_edges(graph)) { 
        for(auto ep = edges(graph); ep.first != ep.second; ++ep.first) {
            ordered_arcs.push_back(graph[*ep.first]);
        }
    
        struct ArcComparer {
            bool operator()(std::shared_ptr<Arc> a1, std::shared_ptr<Arc> a2) {
                return (a2->cost - a1->cost > std::numeric_limits<float>::epsilon());
            }
        } arc_comparer;
    
        sort(ordered_arcs.begin(), ordered_arcs.end(), arc_comparer);
    }
}

std::pair<bool, Vertex> Graph::h1() const {
    return get_vertex_by_node_type(NodeType::H1);
}

std::pair<bool, Vertex> Graph::h2() const {
    return get_vertex_by_node_type(NodeType::H2);
}

std::pair<bool, Vertex> Graph::get_vertex_by_node_type(NodeType n_type) const {
    for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        if(graph[*vp.first]->n_type == n_type) {
            return std::make_pair(true, *vp.first);
        }
    }
    return std::make_pair(false, Vertex());
}

void Graph::prepare_for_labelling() {
    auto i = 0;
    std::vector<std::pair<std::shared_ptr<Port>, PickupType>> checked_ports;
    std::vector<int> pickup_demands;
    std::vector<int> delivery_demands;
    
    for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        auto v = *vp.first;
        graph[v]->boost_vertex_id = i++;
        if(graph[v]->n_type == NodeType::REGULAR_PORT) {
            auto pp = std::make_pair(graph[v]->port, graph[v]->pu_type);
            if(find(checked_ports.begin(), checked_ports.end(), pp) == checked_ports.end()) {
                checked_ports.push_back(pp);
                if(graph[v]->pu_type == PickupType::PICKUP) {
                    pickup_demands.push_back(graph[v]->pu_demand());
                } else {
                    delivery_demands.push_back(graph[v]->de_demand());
                }
            }
        }
    }
    
    graph[graph_bundle].pu_upper_bound = Knapsack::solve(pickup_demands, vessel_class->capacity);
    graph[graph_bundle].de_upper_bound = Knapsack::solve(delivery_demands, vessel_class->capacity);
    
    i = 0;
    for(auto ep = edges(graph); ep.first != ep.second; ++ep.first) {
        graph[*ep.first]->boost_edge_id = i++;
    }
}

void Graph::unite_ports(const VisitRule& vr) {
    std::shared_ptr<Node> n1, n2;
    std::tie(n1, n2) = vr;
    
    if(n1->vessel_class != vessel_class || n2->vessel_class != vessel_class) {
        throw std::runtime_error("Trying to unite ports of nodes not both in this graph");
    }
    
    name = name + " uniting " + n1->port->name + " with " + n2->port->name;
    
    for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        auto v1 = *vp.first;
        
        if(graph[v1]->same_row_as(*n1)) {
            oeit ei, ei_end, ei_next;
            std::tie(ei, ei_end) = out_edges(v1, graph);
            for(ei_next = ei; ei != ei_end; ei = ei_next) {
                ++ei_next;
                auto v2 = target(*ei, graph);
                if(!graph[v2]->same_row_as(*n2)) {
                    remove_edge(*ei, graph);
                }
            }
        } else if(graph[v1]->same_row_as(*n2)) {
            ieit ei, ei_end, ei_next;
            std::tie(ei, ei_end) = in_edges(v1, graph);
            for(ei_next = ei; ei != ei_end; ei = ei_next) {
                ++ei_next;
                auto v2 = source(*ei, graph);
                if(!graph[v2]->same_row_as(*n1)) {
                    remove_edge(*ei, graph);
                }
            }
        }
    }
    
    prepare_for_labelling();
}

void Graph::separate_ports(const VisitRule& vr) {
    std::shared_ptr<Node> n1, n2;
    std::tie(n1, n2) = vr;
    
    if(n1->vessel_class != vessel_class || n2->vessel_class != vessel_class) {
        throw std::runtime_error("Trying to separate ports of nodes not both in this graph");
    }
    
    name = name + " separating " + n1->port->name + " with " + n2->port->name;

    for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        auto v1 = *vp.first;
        
        if(graph[v1]->same_row_as(*n1)) {
            oeit ei, ei_end, ei_next;
            std::tie(ei, ei_end) = out_edges(v1, graph);
            for(ei_next = ei; ei != ei_end; ei = ei_next) {
                ++ei_next;
                auto v2 = target(*ei, graph);
                if(graph[v2]->same_row_as(*n2)) {
                    remove_edge(*ei, graph);
                }
            }
        }
    }
    
    prepare_for_labelling();
}

void Graph::isolate_port(const Node& n) {
    name = name + " isolating " + n.port->name;
    
    for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        if(graph[*vp.first]->same_row_as(n)) {
            clear_vertex(*vp.first, graph);
        }
    }
    
    prepare_for_labelling();
}

std::shared_ptr<Graph> Graph::reduce_graph(float percentage) const {
    if(ordered_arcs.size() != num_edges(graph)) {
        throw std::runtime_error("Trying to reduce a graph whose edges are not sorted");
    }
    
    auto limit_index = (int) (floor((float) num_edges(graph) * percentage));
    auto cost_limit = ordered_arcs[limit_index]->cost;
    auto new_name = name + " reduced for arc cost < " + std::to_string(cost_limit);
    auto dest = std::make_shared<Graph>(graph, vessel_class, new_name);
    
    eit ei, ei_end, ei_next;
    std::tie(ei, ei_end) = edges(dest->graph);
    for(ei_next = ei; ei != ei_end; ei = ei_next) {
        ++ei_next;
        if(dest->graph[*ei]->cost > cost_limit - std::numeric_limits<float>::epsilon()) {
            remove_edge(*ei, dest->graph);
        }
    }
    
    dest->prepare_for_labelling();
    return dest;
}

std::shared_ptr<Graph> Graph::smart_reduce_graph(float min_chance, float max_chance) const {
    auto new_name = name + " reduced smartly";
    auto dest = std::make_shared<Graph>(graph, vessel_class, new_name);
    auto max_prize = max_dual_prize();
    auto min_prize = min_dual_prize();
    
    eit ei, ei_end, ei_next;
    std::tie(ei, ei_end) = edges(dest->graph);
    for(ei_next = ei; ei != ei_end; ei = ei_next) {
        ++ei_next;
        auto trgt = graph[target(*ei, graph)];
        if(trgt->n_type == NodeType::REGULAR_PORT) {
            auto dual_prize = (trgt->pu_type == PickupType::PICKUP ? graph[graph_bundle].port_duals.at(trgt->port).first : graph[graph_bundle].port_duals.at(trgt->port).second);
            auto threshold = min_chance + (dual_prize - min_prize) * (max_chance - min_chance) / (max_prize - min_prize);
            auto rnd = static_cast<float> (rand()) / static_cast<float> (RAND_MAX);
            if(rnd > threshold) {
                remove_edge(*ei, dest->graph);
            }
        }
    }
    
    dest->prepare_for_labelling();
    return dest;
}

float Graph::max_dual_prize() const {
    float max_prize = 0;

    for(const auto& pp : graph[graph_bundle].port_duals) {
        max_prize = std::max(max_prize, pp.second.first);
        max_prize = std::max(max_prize, pp.second.second);
    }
    
    return max_prize;
}

float Graph::min_dual_prize() const {
    auto min_prize = std::numeric_limits<float>::max();
    
    for(const auto& pp : graph[graph_bundle].port_duals) {
        min_prize = std::min(min_prize, pp.second.first);
        min_prize = std::min(min_prize, pp.second.second);
    }
    
    return min_prize;
}

std::pair<bool, Vertex> Graph::get_vertex(const Port& p, PickupType pu, int t) const {
    for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        auto n = *graph[*vp.first];
        if(n.port.get() == &p && n.pu_type == pu && n.time_step == t) {
            return std::make_pair(true, *vp.first);
        }
    }
    
    return std::make_pair(false, Vertex());
}

float Graph::calculate_cost(const Path& p) const {
    auto cost = 0.0f;
    
    for(const auto& e : p) {
        cost += graph[e]->cost;
    }
    
    return cost;
}

float Graph::dual_of(const Node& n) const {
    if(n.n_type == NodeType::REGULAR_PORT) {
        if(n.pu_type == PickupType::PICKUP) {
            return graph[graph_bundle].port_duals.at(n.port).first;
        } else {
            return graph[graph_bundle].port_duals.at(n.port).second;
        }
    } else if(n.n_type == NodeType::H2) {
        return graph[graph_bundle].vc_dual;
    }
    
    return 0;
}

Path Graph::transfer_path(const Path& path, const Graph& subgraph) const {
    Path local_path;
    Path::const_iterator pit;
    for(pit = path.begin(); pit != path.end(); ++pit) {
        auto e = *pit;
        auto n_orig = *subgraph.graph[source(e, subgraph.graph)];
        auto n_dest = *subgraph.graph[target(e, subgraph.graph)];
                
        bool o_found;
        Vertex local_orig;
        std::tie(o_found, local_orig) = get_vertex(*n_orig.port, n_orig.pu_type, n_orig.time_step);
        
        if(!o_found) {
            throw std::runtime_error("In transferring a path from " + subgraph.name + " to " + name + " - " + "Can't find the origin of an edge while transferring paths: " + n_orig.port->name + " at time " + std::to_string(n_orig.time_step));
        }
        
        bool d_found;
        Vertex local_dest;
        std::tie(d_found, local_dest) = get_vertex(*n_dest.port, n_dest.pu_type, n_dest.time_step);
        
        if(!d_found) {
            throw std::runtime_error("In transferring a path from " + subgraph.name + " to " + name + " - " + "Can't find the destination of an edge while transferring paths: " + n_dest.port->name + " at time " + std::to_string(n_dest.time_step));
        }
        
        bool e_found;
        Edge local_edge;
        std::tie(local_edge, e_found) = edge(local_orig, local_dest, graph);
        
        if(!e_found) {
            throw std::runtime_error("In transferring a path from " + subgraph.name + " to " + name + " - " + "The two vertices are not connected in the graph where you want to transfer the path");
        }
        
        local_path.push_back(local_edge);
    }
    
    return local_path;
}