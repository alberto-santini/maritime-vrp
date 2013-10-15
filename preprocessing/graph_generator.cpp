//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <preprocessing/graph_generator.h>

std::shared_ptr<Graph> GraphGenerator::create_graph(const ProblemData& data, std::shared_ptr<VesselClass> vessel_class) {
    std::shared_ptr<Graph> g = make_shared<Graph>(BGraph(), vessel_class, "Graph for " + vessel_class->name);
    
    /*  Add vertices */
    Vertex v_h1, v_h2;
    Node n_h1, n_h2;
    for(std::shared_ptr<Port> p : data.ports) {
        /*  Create nodes H1 and H2 */
        if(p->hub) {
            v_h1 = add_vertex(g->graph);
            g->graph[v_h1] = make_shared<Node>(p, PickupType::PICKUP, NodeType::H1, 0, vessel_class);
            n_h1 = *g->graph[v_h1];
            v_h2 = add_vertex(g->graph);
            g->graph[v_h2] = make_shared<Node>(p, PickupType::DELIVERY, NodeType::H2, data.num_times - 1, vessel_class);
            n_h2 = *g->graph[v_h2];
            continue;
        }
        for(int t = 0; t < data.num_times; t++) {
            for(PickupType pu : {PickupType::PICKUP, PickupType::DELIVERY}) {
                Vertex v = add_vertex(g->graph);
                g->graph[v] = make_shared<Node>(p, pu, NodeType::REGULAR_PORT, t, vessel_class);
            }
        }
    }
    
    /*  Add hub-to-port edges */
    for(std::shared_ptr<Port> p : data.ports) {
        if(p->hub) {
            continue;
        }
        
        if(!p->allowed[vessel_class]) {
            continue;
        }
        
        for(pair<float, float> sc : vessel_class->bunker_cost) {
            float distance = data.distances.at(make_pair(n_h1.port, p));
            float speed = sc.first, u_cost = sc.second;
            int arrival_time = 0 + ceil(distance / speed);
                        
            if(arrival_time >= data.num_times) {
                continue;
            }
            
            int final_time_pu = GraphGenerator::final_time(data, p, arrival_time, PickupType::PICKUP);
                        
            if( (final_time_pu <= GraphGenerator::latest_departure(data, p, n_h2.port, vessel_class)) &&
                (final_time_pu >= data.num_times - 1 - p->pickup_transit)) {
                
                float cost_pu = distance * u_cost + vessel_class->base_cost * (final_time_pu - 0) + p->fee[vessel_class];
                
                GraphGenerator::create_edge(n_h1.port, PickupType::PICKUP, 0,
                                            p, PickupType::PICKUP, final_time_pu,
                                            g, cost_pu);
            }
            
            int final_time_de = GraphGenerator::final_time(data, p, arrival_time, PickupType::DELIVERY);
                        
            if( (final_time_de <= GraphGenerator::latest_departure(data, p, n_h2.port, vessel_class)) &&
                (final_time_de <= p->delivery_transit)) {
                
                float cost_de = distance * u_cost + vessel_class->base_cost * (final_time_de - 0) + p->fee[vessel_class];
                
                GraphGenerator::create_edge(n_h1.port, PickupType::PICKUP, 0,
                                            p, PickupType::DELIVERY, final_time_de,
                                            g, cost_de);
            }
        }
    }
            
    /*  Add port-to-hub edges */
    for(std::shared_ptr<Port> p : data.ports) {
        if(p->hub) {
            continue;
        }
        
        if(!p->allowed[vessel_class]) {
            continue;
        }

        for(pair<float, float> sc : vessel_class->bunker_cost) {
            float distance = data.distances.at(make_pair(p, n_h2.port));
            float speed = sc.first, u_cost = sc.second;
            int departure_time = data.num_times - 1 - ceil(distance / speed);
                        
            if(departure_time < 0) {
                continue;
            }
                        
            bool falls_in_tw = false;
            for(pair<int, int> tw : p->closing_time_windows) {
                if(departure_time > tw.first && departure_time <= tw.second) {
                    falls_in_tw = true;
                    break;
                }
            }
            
            if(falls_in_tw) {
                continue;
            }
                        
            float cost = distance * u_cost + vessel_class->base_cost * ceil(distance / speed);
            
            if(departure_time >= GraphGenerator::earliest_arrival(data, p, n_h1.port, vessel_class)) {
                if(departure_time >= data.num_times - 1 - p->pickup_transit) {
                    GraphGenerator::create_edge(p, PickupType::PICKUP, departure_time,
                                                n_h2.port, PickupType::DELIVERY, data.num_times - 1,
                                                g, cost);
                }
                
                if(departure_time <= p->delivery_transit) {
                    GraphGenerator::create_edge(p, PickupType::DELIVERY, departure_time,
                                                n_h2.port, PickupType::DELIVERY, data.num_times - 1,
                                                g, cost);
                }
            }
        }
    }
                
    /*  Add port-to-port edges */
    for(std::shared_ptr<Port> p : data.ports) {
        if(p->hub) {
            continue;
        }
        
        if(!p->allowed[vessel_class]) {
            continue;
        }
        
        int earliest_arrival = GraphGenerator::earliest_arrival(data, p, n_h1.port, vessel_class);
        int latest_departure = GraphGenerator::latest_departure(data, p, n_h2.port, vessel_class);
        
        for(int t = 0; t < data.num_times - 1; t++) {
            if(t < earliest_arrival || t > latest_departure) {
                continue;
            }
            
            bool falls_in_tw = false;
            for(pair<int, int> tw : p->closing_time_windows) {
                if(t > tw.first && t <= tw.second) {
                    falls_in_tw = true;
                    break;
                }
            }
            
            if(falls_in_tw) {
                continue;
            }
            
            for(PickupType pu : {PickupType::PICKUP, PickupType::DELIVERY}) {
                if(pu == PickupType::PICKUP && t < data.num_times - 1 - p->pickup_transit) {
                    continue;
                }
                if(pu == PickupType::DELIVERY && t > p->delivery_transit) {
                    continue;
                }
                
                for(std::shared_ptr<Port> q : data.ports) {
                    if(p == q) {
                        continue;
                    }
                    
                    if(q->hub) {
                        continue;
                    }
                    
                    if(!q->allowed[vessel_class]) {
                        continue;
                    }
                    
                    for(pair<float, float> sc : vessel_class->bunker_cost) {
                        float distance = data.distances.at(make_pair(p, q));
                        float speed = sc.first, u_cost = sc.second;
                        int arrival_time = t + ceil(distance / speed);
                        
                        if(arrival_time >= data.num_times) {
                            continue;
                        }
                        
                        if(arrival_time < GraphGenerator::earliest_arrival(data, p, n_h1.port, vessel_class)) {
                            continue;
                        }
                        
                        int final_time_pu = GraphGenerator::final_time(data, q, arrival_time, PickupType::PICKUP);
                        
                        if( (final_time_pu <= GraphGenerator::latest_departure(data, q, n_h2.port, vessel_class)) &&
                            (final_time_pu >= data.num_times - 1 - q->pickup_transit)) {
                            
                            float cost_pu = distance * u_cost + vessel_class->base_cost * (final_time_pu - t) + q->fee[vessel_class];
                            
                            GraphGenerator::create_edge(p, pu, t,
                                                        q, PickupType::PICKUP, final_time_pu,
                                                        g, cost_pu);

                        }
                        
                        int final_time_de = GraphGenerator::final_time(data, q, arrival_time, PickupType::DELIVERY);
                        
                        if( (final_time_de <= GraphGenerator::latest_departure(data, q, n_h2.port, vessel_class)) &&
                            (final_time_de <= q->delivery_transit)) {
                            
                            float cost_de = distance * u_cost + vessel_class->base_cost * (final_time_de - t) + q->fee[vessel_class];
                            
                            GraphGenerator::create_edge(p, pu, t,
                                                        q, PickupType::DELIVERY, final_time_pu,
                                                        g, cost_de);
                            
                        }
                    }
                }
            }
        }
    }
            
    /*  Add delivery-to-pickup edges */
    pair<vit, vit> vp;
    for(vp = vertices(g->graph); vp.first != vp.second; ++vp.first) {
        Node n = *g->graph[*vp.first];
        if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PickupType::DELIVERY) {
            GraphGenerator::create_edge(n.port, n.pu_type, n.time_step,
                                        n.port, PickupType::PICKUP, n.time_step,
                                        g, 0);
        }
    }
        
    /*  Do some spring cleaning */
    bool clean = false;
    while(!clean) {
        clean = true;
        vit vi, vi_end, vi_next;
        tie(vi, vi_end) = vertices(g->graph);
        for(vi_next = vi; vi != vi_end; vi = vi_next) {
            ++vi_next;
            int n_out = (int)out_degree(*vi, g->graph);
            int n_in = (int)in_degree(*vi, g->graph);
            if((g->graph[*vi]->n_type == NodeType::REGULAR_PORT) && (n_out == 0 || n_in == 0 || n_out + n_in <= 1)) {
                clear_vertex(*vi, g->graph);
                remove_vertex(*vi, g->graph);
                clean = false;
            }
        }
    }
    
    g->prepare_for_labelling();
    
    return g;
}

int GraphGenerator::final_time(const ProblemData& data, std::shared_ptr<Port> p, const int arrival_time, const PickupType pu) {
    int ft_handling = arrival_time + (pu == PickupType::PICKUP ? p->pickup_handling : p->delivery_handling);
    int ft = ft_handling;
    bool free_from_tw = false;
        
    while(!free_from_tw) {
        free_from_tw = true;
        for(pair<int, int> tw : p->closing_time_windows) {
            int t1 = tw.first, t2 = tw.second;
            if(ft <= t2 && ft > t1) {
                ft += (t2 - t1);
                free_from_tw = false;
            }
        }
    }
    
    return min(ft, data.num_times - 1);
}

int GraphGenerator::latest_departure(const ProblemData& data, std::shared_ptr<Port> p, std::shared_ptr<Port> h2, std::shared_ptr<VesselClass> vessel_class) {
    float top_speed = vessel_class->top_speed;
    return (data.num_times - floor(data.distances.at(make_pair(p, h2)) / top_speed));
}

int GraphGenerator::earliest_arrival(const ProblemData& data, std::shared_ptr<Port> p, std::shared_ptr<Port> h1, std::shared_ptr<VesselClass> vessel_class) {
    float top_speed = vessel_class->top_speed;
    return ceil(data.distances.at(make_pair(h1, p)) / top_speed);
}

void GraphGenerator::create_edge(std::shared_ptr<Port> origin_p, const PickupType origin_pu, const int origin_t,
                                 std::shared_ptr<Port> destination_p, const PickupType destination_pu, const int destination_t,
                                 std::shared_ptr<Graph> g, const float cost) {
    bool origin_found, destination_found;
    Vertex origin_v, destination_v;

    tie(origin_found, origin_v) = g->get_vertex(origin_p, origin_pu, origin_t);
    
    if(!origin_found) {
        throw runtime_error("Can't find the origin vertex: " + origin_p->name + " at time " + to_string(origin_t));
    }
    
    tie(destination_found, destination_v) = g->get_vertex(destination_p, destination_pu, destination_t);
    
    if(!destination_found) {
        throw runtime_error("Can't find the destination vertex: " + destination_p->name + " at time " + to_string(destination_t));
    }
    
    Edge e = add_edge(origin_v, destination_v, g->graph).first;
    g->graph[e] = make_shared<Arc>(cost);
}