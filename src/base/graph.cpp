//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <fstream>

#include <base/graph.h>

void Graph::dump() const {
    std::ofstream gfile;
    gfile.open("graph.txt", std::ios::out);
    
    gfile << num_vertices(graph) << " #" << vessel_class->name << std::endl;
    
    for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        const Node& node = *graph[*vp.first];
        
        auto desc = "reg";
        if(node.n_type == NodeType::H1) { desc = "src"; }
        if(node.n_type == NodeType::H2) { desc = "snk"; }
        
        auto pickup = 0u;
        if(node.pu_type == PickupType::PICKUP) { pickup = node.port->pickup_demand; }
        
        auto delivery = 0u;
        if(node.pu_type == PickupType::DELIVERY) { delivery = node.port->delivery_demand; }
        
        gfile <<
            node.boost_vertex_id << "\t" <<
            desc << "\t" <<
            pickup << "\t" <<
            delivery << "\t" <<
            dual_of(node) << "\t" <<
            node.pu_penalty() + node.de_penalty() << std::endl;
    }
    
    for(auto ep = edges(graph); ep.first != ep.second; ++ep.first) {
        const Arc& arc = *graph[*ep.first];
        const Node& src_n = *graph[source(*ep.first, graph)];
        const Node& dst_n = *graph[target(*ep.first, graph)];
        
        gfile <<
            arc.boost_edge_id << "\t" <<
            src_n.boost_vertex_id << "\t" <<
            dst_n.boost_vertex_id << "\t" <<
            arc.cost << std::endl;
    }
    
    gfile.close();
}

void Graph::print(bool detailed) const {
    std::cout << "Graph: " << name << std::endl;
    std::cout << "Number of vertices: " << num_vertices(graph) << std::endl;
    std::cout << "Number of edges: " << num_edges(graph) << std::endl;
    
    if(detailed) {
        std::cout << "Vertices:" << std::endl;
        for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
            const Node& n = *graph[*vp.first];
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
                return (a2->cost > a1->cost);
            }
        } arc_comparer;
    
        std::sort(ordered_arcs.begin(), ordered_arcs.end(), arc_comparer);
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
    
    for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        graph[*vp.first]->boost_vertex_id = i++;
    }
    
    i = 0;
    for(auto ep = edges(graph); ep.first != ep.second; ++ep.first) {
        graph[*ep.first]->boost_edge_id = i++;
    }
}

ErasedEdges Graph::get_erased_edges_from_rules(ErasedEdges already_erased, const VisitRuleList& unite_rules, const VisitRuleList& separate_rules) const {
    ErasedEdges erased = already_erased;
    
    for(const auto& vr : unite_rules) {
        std::shared_ptr<Node> n1, n2;
        std::tie(n1, n2) = vr;
        
        for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
            const Vertex& v1 = *vp.first;
        
            if(graph[v1]->same_row_as(*n1)) {
                oeit ei, ei_end, ei_next;
                std::tie(ei, ei_end) = out_edges(v1, graph);
                for(ei_next = ei; ei != ei_end; ei = ei_next) {
                    ++ei_next;
                    const Vertex& v2 = target(*ei, graph);
                    if(!graph[v2]->same_row_as(*n2)) {
                        if(erased.find(v1) == erased.end()) { erased[v1] = std::set<Edge>(); }
                        erased[v1].insert(*ei);
                    }
                }
            } else if(graph[v1]->same_row_as(*n2)) {
                ieit ei, ei_end, ei_next;
                std::tie(ei, ei_end) = in_edges(v1, graph);
                for(ei_next = ei; ei != ei_end; ei = ei_next) {
                    ++ei_next;
                    const Vertex& v2 = source(*ei, graph);
                    if(!graph[v2]->same_row_as(*n1)) {
                        if(erased.find(v2) == erased.end()) { erased[v2] = std::set<Edge>(); }
                        erased[v2].insert(*ei);
                    }
                }
            }
        }
    }
    
    for(const auto& vr : separate_rules) {
        std::shared_ptr<Node> n1, n2;
        std::tie(n1, n2) = vr;
    
        for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
            const Vertex& v1 = *vp.first;
        
            if(graph[v1]->same_row_as(*n1)) {
                oeit ei, ei_end, ei_next;
                std::tie(ei, ei_end) = out_edges(v1, graph);
                for(ei_next = ei; ei != ei_end; ei = ei_next) {
                    ++ei_next;
                    const Vertex& v2 = target(*ei, graph);
                    if(graph[v2]->same_row_as(*n2)) {
                        if(erased.find(v1) == erased.end()) { erased[v1] = std::set<Edge>(); }
                        erased[v1].insert(*ei);
                    }
                }
            }
        }
    }
    
    return erased;
}

ErasedEdges Graph::reduce_graph(double percentage, ErasedEdges already_erased) const {
    ErasedEdges erased = already_erased;
    auto n_erased = std::accumulate(erased.begin(), erased.end(), 0u, [] (auto sum, const auto& ve) { return sum + ve.second.size(); });
    
    if(ordered_arcs.size() != num_edges(graph)) {
        throw std::runtime_error("Trying to reduce a graph whose edges are not sorted");
    }
    
    auto limit_index = (int) (floor((double) (num_edges(graph) - n_erased) * percentage));
    auto cost_limit = ordered_arcs[limit_index]->cost;
    
    eit ei, ei_end, ei_next;
    std::tie(ei, ei_end) = edges(graph);
    for(ei_next = ei; ei != ei_end; ei = ei_next) {
        ++ei_next;
        if(graph[*ei]->cost > cost_limit) {
            const Vertex& src = source(*ei, graph);
            if(erased.find(src) == erased.end()) { erased[src] = std::set<Edge>(); }
            erased[src].insert(*ei);
        }
    }
    
    return erased;
}

ErasedEdges Graph::smart_reduce_graph(double min_chance, double max_chance, ErasedEdges already_erased) const {
    ErasedEdges erased = already_erased;

    auto max_prize = max_dual_prize();
    auto min_prize = min_dual_prize();

    eit ei, ei_end, ei_next;
    std::tie(ei, ei_end) = edges(graph);
    for(ei_next = ei; ei != ei_end; ei = ei_next) {
        ++ei_next;
        const Node& trgt = *graph[target(*ei, graph)];
        if(trgt.n_type == NodeType::REGULAR_PORT) {
            auto dual_prize = dual_of(trgt);
            auto threshold = min_chance + (dual_prize - min_prize) * (max_chance - min_chance) / (max_prize - min_prize);
            auto rnd = static_cast<double> (rand()) / static_cast<double> (RAND_MAX);
            if(rnd > threshold) {
                const Vertex& src = source(*ei, graph);
                if(erased.find(src) == erased.end()) { erased[src] = std::set<Edge>(); }
                erased[src].insert(*ei);
            }
        }
    }

    return erased;
}

double Graph::max_dual_prize() const {
    double max_prize = 0;

    for(const auto& pp : graph[graph_bundle].port_duals) {
        max_prize = std::max(max_prize, pp.second.first);
        max_prize = std::max(max_prize, pp.second.second);
    }
    
    return max_prize;
}

double Graph::min_dual_prize() const {
    auto min_prize = std::numeric_limits<double>::max();
    
    for(const auto& pp : graph[graph_bundle].port_duals) {
        min_prize = std::min(min_prize, pp.second.first);
        min_prize = std::min(min_prize, pp.second.second);
    }
    
    return min_prize;
}

std::pair<bool, Vertex> Graph::get_vertex(const Port& p, PickupType pu, int t) const {
    for(auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        const Node& n = *graph[*vp.first];
        if(n.port.get() == &p && n.pu_type == pu && n.time_step == t) {
            return std::make_pair(true, *vp.first);
        }
    }
    
    return std::make_pair(false, Vertex());
}

double Graph::calculate_cost(const Path& p) const {
    auto cost = 0.0f;
    
    for(const auto& e : p) {
        cost += graph[e]->cost;
    }
    
    return cost;
}

double Graph::dual_of(const Node& n) const {
    if(n.n_type == NodeType::REGULAR_PORT) {
        auto it = graph[graph_bundle].port_duals.find(n.port);
        
        if(n.pu_type == PickupType::PICKUP) {
            return (it == graph[graph_bundle].port_duals.end() ? 0 : it->second.first);
        } else {
            return (it == graph[graph_bundle].port_duals.end() ? 0 : it->second.second);
        }
    } else if(n.n_type == NodeType::H2) {
        return graph[graph_bundle].vc_dual;
    }
    
    return 0;
}