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
            Node n = *graph[*vp.first];
            string prize;
            try {
                prize = to_string(dual_of(n));
            } catch(std::out_of_range& e) {
                prize = "undefined";
            }
            cout << n << " - Dual prize/price: " << prize << endl;
        }
        cout << "Edges:" << endl;
        pair<eit, eit> ep;
        for(ep = edges(graph); ep.first != ep.second; ++ep.first) {
            cout << *graph[source(*ep.first, graph)] << " -> " << *graph[target(*ep.first, graph)];
            cout << " - Cost: " << graph[*ep.first]->cost << endl;
        }
    }
}

void Graph::print_path(const Path& p, ostream& out) const {
    Path::const_reverse_iterator pit;
    for(pit = p.rbegin(); pit != p.rend(); ++pit) {
        out << *graph[source(*pit, graph)] << " -> " << *graph[target(*pit, graph)] << endl;
    }
}

void Graph::sort_arcs() {
    if(ordered_arcs.size() != num_edges(graph)) { 
        pair<eit, eit> ep;
        for(ep = edges(graph); ep.first != ep.second; ++ep.first) {
            ordered_arcs.push_back(graph[*ep.first]);
        }
    
        struct ArcComparer {
            bool operator()(std::shared_ptr<Arc> a1, std::shared_ptr<Arc> a2) {
                return (a2->cost - a1->cost > numeric_limits<float>::epsilon());
            }
        } arc_comparer;
    
        sort(ordered_arcs.begin(), ordered_arcs.end(), arc_comparer);
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
    pair<eit, eit> ep;
    for(ep = edges(graph); ep.first != ep.second; ++ep.first) {
        graph[*ep.first]->boost_edge_id = i++;
    }
}

void Graph::unite_ports(VisitRule vr) {
    std::shared_ptr<Node> n1, n2;
    tie(n1, n2) = vr;
    
    //if(n1->n_type != NodeType::REGULAR_PORT || n2->n_type != NodeType::REGULAR_PORT) {
    //    throw runtime_error("Trying to unite ports of nodes that are not both regular nodes");
    //}
    
    if(n1->vessel_class != vessel_class || n2->vessel_class != vessel_class) {
        throw runtime_error("Trying to unite ports of nodes not both in this graph");
    }
    
    name = name + " uniting " + n1->port->name + " with " + n2->port->name;
    
    pair<vit, vit> vp;
    for(vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        Vertex v1 = *vp.first;
        
        if(graph[v1]->same_row_as(*n1)) {
            oeit ei, ei_end, ei_next;
            tie(ei, ei_end) = out_edges(v1, graph);
            for(ei_next = ei; ei != ei_end; ei = ei_next) {
                ++ei_next;
                Vertex v2 = target(*ei, graph);
                if(!graph[v2]->same_row_as(*n2)) {
                    remove_edge(*ei, graph);
                }
            }
        } else if(graph[v1]->same_row_as(*n2)) {
            ieit ei, ei_end, ei_next;
            tie(ei, ei_end) = in_edges(v1, graph);
            for(ei_next = ei; ei != ei_end; ei = ei_next) {
                ++ei_next;
                Vertex v2 = source(*ei, graph);
                if(!graph[v2]->same_row_as(*n1)) {
                    remove_edge(*ei, graph);
                }
            }
        }
    }
    
    prepare_for_labelling();
}

void Graph::separate_ports(VisitRule vr) {
    std::shared_ptr<Node> n1, n2;
    tie(n1, n2) = vr;
    
    //if(n1->n_type != NodeType::REGULAR_PORT || n2->n_type != NodeType::REGULAR_PORT) {
    //    throw runtime_error("Trying to unite ports of nodes that are not both regular nodes");
    //}
    
    if(n1->vessel_class != vessel_class || n2->vessel_class != vessel_class) {
        throw runtime_error("Trying to unite ports of nodes not both in this graph");
    }
    
    name = name + " separating " + n1->port->name + " with " + n2->port->name;

    pair<vit, vit> vp;
    for(vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        Vertex v1 = *vp.first;
        
        if(graph[v1]->same_row_as(*n1)) {
            oeit ei, ei_end, ei_next;
            tie(ei, ei_end) = out_edges(v1, graph);
            for(ei_next = ei; ei != ei_end; ei = ei_next) {
                ++ei_next;
                Vertex v2 = target(*ei, graph);
                if(graph[v2]->same_row_as(*n2)) {
                    remove_edge(*ei, graph);
                }
            }
        }
    }
    
    prepare_for_labelling();
}

void Graph::isolate_port(std::shared_ptr<Node> n) {
    name = name + " isolating " + n->port->name;
    
    pair<vit, vit> vp;
    for(vp = vertices(graph); vp.first != vp.second; ++vp.first) {
        if(graph[*vp.first]->same_row_as(*n)) {
            clear_vertex(*vp.first, graph);
        }
    }
    
    prepare_for_labelling();
}

std::shared_ptr<Graph> Graph::reduce_graph(const float percentage) const {
    if(ordered_arcs.size() != num_edges(graph)) {
        throw runtime_error("Trying to reduce a graph whose edges are not sorted");
    }
    
    int limit_index = floor((float) num_edges(graph) * percentage);
    float cost_limit = ordered_arcs[limit_index]->cost;
    string new_name = name + " reduced for arc cost < " + to_string(cost_limit);
    std::shared_ptr<Graph> dest = make_shared<Graph>(graph, vessel_class, new_name);
    
    eit ei, ei_end, ei_next;
    tie(ei, ei_end) = edges(dest->graph);
    for(ei_next = ei; ei != ei_end; ei = ei_next) {
        ++ei_next;
        if(dest->graph[*ei]->cost > cost_limit - numeric_limits<float>::epsilon()) {
            remove_edge(*ei, dest->graph);
        }
    }
    
    dest->prepare_for_labelling();
    return dest;
}

std::shared_ptr<Graph> Graph::smart_reduce_graph(const float min_chance, const float max_chance) const {
    string new_name = name + " reduced smartly";
    std::shared_ptr<Graph> dest = make_shared<Graph>(graph, vessel_class, new_name);
    float max_prize = max_dual_prize();
    float min_prize = min_dual_prize();
    
    eit ei, ei_end, ei_next;
    tie(ei, ei_end) = edges(dest->graph);
    for(ei_next = ei; ei != ei_end; ei = ei_next) {
        ++ei_next;
        std::shared_ptr<Node> trgt = graph[target(*ei, graph)];
        if(trgt->n_type == NodeType::REGULAR_PORT) {
            float dual_prize = (trgt->pu_type == PickupType::PICKUP ? graph[graph_bundle].port_duals.at(trgt->port).first : graph[graph_bundle].port_duals.at(trgt->port).second);
            float threshold = min_chance + (dual_prize - min_prize) * (max_chance - min_chance) / (max_prize - min_prize);
            float rnd = static_cast<float> (rand()) / static_cast<float> (RAND_MAX);
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
        max_prize = max(max_prize, pp.second.first);
        max_prize = max(max_prize, pp.second.second);
    }
    
    return max_prize;
}

float Graph::min_dual_prize() const {
    float min_prize = numeric_limits<float>::max();
    
    for(const auto& pp : graph[graph_bundle].port_duals) {
        min_prize = min(min_prize, pp.second.first);
        min_prize = min(min_prize, pp.second.second);
    }
    
    return min_prize;
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
    
    for(const Edge& e : p) {
        cost += graph[e]->cost;
    }
    
    return cost;
}

float Graph::dual_of(const Node n) const {
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

Path Graph::transfer_path(const Path& path, const std::shared_ptr<const Graph> subgraph) const {
    Path local_path;
    Path::const_iterator pit;
    for(pit = path.begin(); pit != path.end(); ++pit) {
        Edge e = *pit;
        Node n_orig = *subgraph->graph[source(e, subgraph->graph)];
        Node n_dest = *subgraph->graph[target(e, subgraph->graph)];
                
        bool o_found;
        Vertex local_orig;
        tie(o_found, local_orig) = get_vertex(n_orig.port, n_orig.pu_type, n_orig.time_step);
        
        if(!o_found) {
            throw runtime_error("In transferring a path from " + subgraph->name + " to " + name + " - " + "Can't find the origin of an edge while transferring paths: " + n_orig.port->name + " at time " + to_string(n_orig.time_step));
        }
        
        bool d_found;
        Vertex local_dest;
        tie(d_found, local_dest) = get_vertex(n_dest.port, n_dest.pu_type, n_dest.time_step);
        
        if(!d_found) {
            throw runtime_error("In transferring a path from " + subgraph->name + " to " + name + " - " + "Can't find the destination of an edge while transferring paths: " + n_dest.port->name + " at time " + to_string(n_dest.time_step));
        }
        
        bool e_found;
        Edge local_edge;
        tie(local_edge, e_found) = edge(local_orig, local_dest, graph);
        
        if(!e_found) {
            throw runtime_error("In transferring a path from " + subgraph->name + " to " + name + " - " + "The two vertices are not connected in the graph where you want to transfer the path");
        }
        
        local_path.push_back(local_edge);
    }
    
    return local_path;
}