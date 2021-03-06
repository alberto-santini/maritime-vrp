//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <string>
#include <utility>

#include "graph_generator.h"

namespace mvrp {
    namespace GraphGenerator {
        std::shared_ptr<Graph>
        create_graph(const ProblemData &data, const ProgramParams &params, std::shared_ptr<VesselClass> vessel_class) {
            auto g = std::make_shared<Graph>(BGraph(), vessel_class);
            auto created_pu = std::unordered_map<std::shared_ptr<Port>, bool>();
            auto created_de = std::unordered_map<std::shared_ptr<Port>, bool>();
            const Port* comeback_hub_port = nullptr;

            /*  Add vertices */
            Vertex v_h1, v_h2;
            Node n_h1, n_h2;
            for(auto p : data.ports) {
                /*  Create source and sink nodes */
                if(p->hub) {
                    v_h1 = add_vertex(g->graph);
                    g->graph[v_h1] = std::make_shared<Node>(p, PortType::PICKUP, NodeType::SOURCE_VERTEX, 0, vessel_class);
                    n_h1 = *g->graph[v_h1];

                    v_h2 = add_vertex(g->graph);
                    g->graph[v_h2] = std::make_shared<Node>(p, PortType::DELIVERY, NodeType::SINK_VERTEX, data.num_times - 1, vessel_class);
                    n_h2 = *g->graph[v_h2];

                    /* Create come-back hub nodes */
                    for(auto t = 1; t < data.num_times - 1; t++) {
                        auto v = add_vertex(g->graph);
                        g->graph[v] = std::make_shared<Node>(p, PortType::BOTH, NodeType::COMEBACK_HUB, t, vessel_class);
                    }

                    assert(comeback_hub_port == nullptr);
                    comeback_hub_port = p.get();

                    continue;
                }

                if(p->allowed[vessel_class]) {
                    /* Create all other nodes */
                    if(p->pickup_demand > 0 && p->pickup_demand <= vessel_class->capacity) {
                        for(auto t = 0; t < data.num_times; t++) {
                            auto v = add_vertex(g->graph);
                            g->graph[v] = std::make_shared<Node>(p, PortType::PICKUP, NodeType::REGULAR_PORT, t, vessel_class);
                        }

                        created_pu[p] = true;
                    } else {
                        created_pu[p] = false;
                    }

                    if(p->delivery_demand > 0 && p->delivery_demand <= vessel_class->capacity) {
                        for(auto t = 0; t < data.num_times; t++) {
                            auto v = add_vertex(g->graph);
                            g->graph[v] = std::make_shared<Node>(p, PortType::DELIVERY, NodeType::REGULAR_PORT, t, vessel_class);
                        }

                        created_de[p] = true;
                    } else {
                        created_de[p] = false;
                    }
                }
            }

            assert(comeback_hub_port != nullptr);

            /*  Add hub-to-port edges */
            for(auto p : data.ports) {
                if(p->hub) {
                    continue;
                }

                if(!p->allowed[vessel_class]) {
                    continue;
                }

                for(const auto &sc : vessel_class->bunker_cost_per_time_unit) {
                    auto distance = data.distances.at(std::make_pair(n_h1.port, p));
                    auto speed = sc.first;
                    auto bunker_cost_per_time_unit = sc.second;
                    auto arrival_time = 0 + (int) ceil(distance / speed);

                    if(arrival_time >= data.num_times) {
                        continue;
                    }

                    if(created_pu[p]) {
                        auto final_time_pu = final_time(data, *p, arrival_time, PortType::PICKUP);
                        auto movement_cost = p->pickup_movement_cost;
                        auto fixed_port_fee = p->fixed_fee;
                        auto variable_port_fee = p->variable_fee[vessel_class];
                        auto revenue = p->pickup_revenue;

                        if(final_time_pu <= latest_departure(data, p, n_h2.port, *vessel_class)) {
                            auto bunker_cost = (arrival_time - 0) * bunker_cost_per_time_unit;

                            if(final_time_pu >= data.num_times - 1 - p->pickup_transit) {
                                // Normal arc: arrives at a time when it's allowed
                                auto time_charter_cost = (final_time_pu - 0) * vessel_class->time_charter_cost_per_time_unit;
                                auto hotel_cost = (final_time_pu - arrival_time) * vessel_class->hotel_cost_per_time_unit;

                                create_edge(*n_h1.port, PortType::PICKUP, 0, *p, PortType::PICKUP, final_time_pu, g,
                                            bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                            fixed_port_fee + variable_port_fee, revenue, distance,
                                            speed, ArcType::SOURCE_TO_PORT);
                            } else {
                                // Travel + wait arc: arrives at a time when it's not allowed
                                auto overall_final_time = data.num_times - 1 - p->pickup_transit;

                                auto time_charter_cost = (overall_final_time - 0) * vessel_class->time_charter_cost_per_time_unit;
                                auto hotel_cost = (overall_final_time - arrival_time) * vessel_class->hotel_cost_per_time_unit;

                                create_edge(*n_h1.port, PortType::PICKUP, 0, *p, PortType::PICKUP,
                                            overall_final_time, g,
                                            bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                            fixed_port_fee + variable_port_fee, revenue, distance,
                                            speed, ArcType::SOURCE_TO_PORT);
                            }
                        }

                        /* Create arcs from comeback-hub to port */
                        for(auto t = 1; t < data.num_times - 1; t++) {
                            auto comeback_arrival_time = t + arrival_time;

                            if(comeback_arrival_time >= data.num_times) { continue; }

                            auto comeback_final_time_pu = final_time(data, *p, comeback_arrival_time, PortType::PICKUP);

                            if(comeback_final_time_pu <= latest_departure(data, p, n_h2.port, *vessel_class)) {
                                auto bunker_cost = (comeback_arrival_time - t) * bunker_cost_per_time_unit;

                                if(comeback_final_time_pu >= data.num_times - 1 - p->pickup_transit) {
                                    // Normal arc: arrives at a time when it's allowed
                                    auto time_charter_cost = (comeback_final_time_pu - t) * vessel_class->time_charter_cost_per_time_unit;
                                    auto hotel_cost = (comeback_final_time_pu - comeback_arrival_time) * vessel_class->hotel_cost_per_time_unit;

                                    create_edge(*comeback_hub_port, PortType::BOTH, t, *p, PortType::PICKUP, comeback_final_time_pu,
                                                g, bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                                fixed_port_fee + variable_port_fee, revenue, distance,
                                                speed, ArcType::COMEBACK_HUB_TO_PORT);
                                } else {
                                    // Travel + wait arc: arrives at a time when it's not allowed
                                    auto comeback_overall_final_time = data.num_times - 1 - p->pickup_transit;

                                    auto time_charter_cost = (comeback_overall_final_time - t) * vessel_class->time_charter_cost_per_time_unit;
                                    auto hotel_cost = (comeback_overall_final_time - comeback_arrival_time) * vessel_class->hotel_cost_per_time_unit;

                                    create_edge(*comeback_hub_port, PortType::BOTH, t, *p, PortType::PICKUP, comeback_overall_final_time,
                                                g, bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                                fixed_port_fee + variable_port_fee, revenue, distance,
                                                speed, ArcType::COMEBACK_HUB_TO_PORT);
                                }
                            }
                        }
                    }

                    if(created_de[p]) {
                        auto final_time_de = final_time(data, *p, arrival_time, PortType::DELIVERY);
                        auto movement_cost = p->delivery_movement_cost;
                        auto fixed_port_fee = p->fixed_fee;
                        auto variable_port_fee = p->variable_fee[vessel_class];
                        auto revenue = p->delivery_revenue;

                        if((final_time_de <= latest_departure(data, p, n_h2.port, *vessel_class)) &&
                           (final_time_de <= p->delivery_transit)) {

                            auto time_charter_cost = (final_time_de - 0) * vessel_class->time_charter_cost_per_time_unit;
                            auto hotel_cost = (final_time_de - arrival_time) * vessel_class->hotel_cost_per_time_unit;
                            auto bunker_cost = (arrival_time - 0) * bunker_cost_per_time_unit;

                            create_edge(*n_h1.port, PortType::PICKUP, 0, *p, PortType::DELIVERY, final_time_de, g,
                                        bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                        fixed_port_fee + variable_port_fee, revenue, distance,
                                        speed, ArcType::SOURCE_TO_PORT);
                        }

                        /* Create arcs from comeback-hub to port */
                        for(auto t = 1; t < data.num_times - 1; t++) {
                            auto comeback_arrival_time = arrival_time + t;
                            auto comeback_final_time_de = final_time(data, *p, comeback_arrival_time, PortType::DELIVERY);

                            if((comeback_final_time_de <= latest_departure(data, p, n_h2.port, *vessel_class)) &&
                               (comeback_final_time_de <= p->delivery_transit)) {

                                auto time_charter_cost = (comeback_final_time_de - t) * vessel_class->time_charter_cost_per_time_unit;
                                auto hotel_cost = (comeback_final_time_de - comeback_arrival_time) * vessel_class->hotel_cost_per_time_unit;
                                auto bunker_cost = (comeback_arrival_time - t) * bunker_cost_per_time_unit;

                                create_edge(*comeback_hub_port, PortType::BOTH, t, *p, PortType::DELIVERY, comeback_final_time_de,
                                            g, bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                            fixed_port_fee + variable_port_fee, revenue, distance,
                                            speed, ArcType::COMEBACK_HUB_TO_PORT);
                            }
                        }
                    }
                }
            }

            /*  Add port-to-hub edges */
            for(auto p : data.ports) {
                if(p->hub || !p->allowed[vessel_class]) {
                    continue;
                }

                auto pickup_departure_time = std::max(earliest_arrival(data, p, n_h1.port, *vessel_class), data.num_times - 1 - p->pickup_transit);
                auto delivery_departure_time = std::max(earliest_arrival(data, p, n_h1.port, *vessel_class), p->delivery_transit);

                assert(pickup_departure_time >= 1);
                assert(delivery_departure_time >= 1);

                if(created_pu[p]) {
                    // For pickup nodes:
                    for(auto departure_time = pickup_departure_time; departure_time < latest_departure(data, p, n_h2.port, *vessel_class); ++departure_time) {
                        auto falls_in_tw = false;
                        for(const auto &tw : p->closing_time_windows) {
                            if(departure_time > tw.first && departure_time <= tw.second) {
                                falls_in_tw = true;
                                break;
                            }
                        }

                        if(falls_in_tw) {
                            continue;
                        }

                        for(const auto &sc : vessel_class->bunker_cost_per_time_unit) {
                            auto distance = data.distances.at(std::make_pair(p, n_h2.port));
                            auto speed = sc.first;
                            auto bunker_cost_per_time_unit = sc.second;
                            auto arrival_time = departure_time + (int) ceil(distance / speed);
                            auto final_time = data.num_times - 1;

                            if(arrival_time >= data.num_times) {
                                continue;
                            }

                            // The following works the same for:
                            // 1) When arrival_time == data.num_times - 1 => arrival at exact time => no waiting at hub
                            // 2) When arrival_time < data.num_times - 1 => early arrival => waiting at the hub
                            auto time_charter_cost = (final_time - departure_time) * vessel_class->time_charter_cost_per_time_unit;
                            auto hotel_cost = (final_time - arrival_time) * vessel_class->hotel_cost_per_time_unit;
                            auto bunker_cost = (arrival_time - departure_time) * bunker_cost_per_time_unit;
                            auto movement_cost = 0;
                            auto fixed_port_fee = n_h2.port->fixed_fee;
                            auto variable_port_fee = n_h2.port->variable_fee[vessel_class];
                            auto revenue = 0;

                            create_edge(*p, PortType::PICKUP, departure_time, *n_h2.port, PortType::DELIVERY,
                                        data.num_times - 1, g,
                                        bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                        fixed_port_fee + variable_port_fee, revenue, distance,
                                        speed, ArcType::PORT_TO_SINK);

                            /* Create arc from the port to the comeback-hub */
                            // We add an fixed slack of 2 time units for operations at the hub
                            if(arrival_time + 2 < data.num_times) {
                                create_edge(*p, PortType::PICKUP, departure_time, *comeback_hub_port, PortType::BOTH,
                                            arrival_time + 2, g, bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                            fixed_port_fee + variable_port_fee, revenue, distance,
                                            speed, ArcType::PORT_TO_COMEBACK_HUB);
                            }
                        }
                    }
                }

                if(created_de[p]) {
                    // For delivery nodes:
                    for(auto departure_time = delivery_departure_time;
                        departure_time < latest_departure(data, p, n_h2.port, *vessel_class); ++departure_time) {
                        auto falls_in_tw = false;
                        for(const auto &tw : p->closing_time_windows) {
                            if(departure_time > tw.first && departure_time <= tw.second) {
                                falls_in_tw = true;
                                break;
                            }
                        }

                        if(falls_in_tw) {
                            continue;
                        }

                        for(const auto &sc : vessel_class->bunker_cost_per_time_unit) {
                            auto distance = data.distances.at(std::make_pair(p, n_h2.port));
                            auto speed = sc.first;
                            auto bunker_cost_per_time_unit = sc.second;
                            auto arrival_time = departure_time + (int) ceil(distance / speed);
                            auto final_time = data.num_times - 1;

                            if(arrival_time >= data.num_times) {
                                continue;
                            }

                            // Thew following works the same for:
                            // 1) When arrival_time == data.num_times - 1 => arrival at exact time => no waiting at hub
                            // 2) When arrival_time < data.num_times - 1 => early arrival => waiting at the hub
                            auto time_charter_cost = (final_time - departure_time) * vessel_class->time_charter_cost_per_time_unit;
                            auto hotel_cost = (final_time - arrival_time) * vessel_class->hotel_cost_per_time_unit;
                            auto bunker_cost = (arrival_time - departure_time) * bunker_cost_per_time_unit;
                            auto movement_cost = 0;
                            auto fixed_port_fee = n_h2.port->fixed_fee;
                            auto variable_port_fee = n_h2.port->variable_fee[vessel_class];
                            auto revenue = 0;

                            create_edge(*p, PortType::DELIVERY, departure_time, *n_h2.port, PortType::DELIVERY,
                                        data.num_times - 1, g,
                                        bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                        fixed_port_fee + variable_port_fee, revenue, distance,
                                        speed, ArcType::PORT_TO_SINK);

                            /* Create arc from the port to the comeback-hub */
                            // We add an fixed slack of 2 time units for operations at the hub
                            if(arrival_time + 2 < data.num_times) {
                                create_edge(*p, PortType::DELIVERY, departure_time, *comeback_hub_port, PortType::BOTH,
                                            arrival_time + 2, g, bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                            fixed_port_fee + variable_port_fee, revenue, distance,
                                            speed, ArcType::PORT_TO_COMEBACK_HUB);
                            }
                        }
                    }
                }
            }

            /*  Add port-to-port edges */
            for(auto p : data.ports) {
                if(p->hub) {
                    continue;
                }

                if(!p->allowed[vessel_class]) {
                    continue;
                }

                for(auto t = earliest_arrival(data, p, n_h1.port, *vessel_class); t <= latest_departure(data, p, n_h2.port, *vessel_class); t++) {
                    auto falls_in_tw = false;
                    for(const auto &tw : p->closing_time_windows) {
                        if(t > tw.first && t <= tw.second) {
                            falls_in_tw = true;
                            break;
                        }
                    }

                    if(falls_in_tw) {
                        continue;
                    }

                    for(auto pu : {PortType::PICKUP, PortType::DELIVERY}) {
                        if(pu == PortType::PICKUP && (!created_pu[p] || t < data.num_times - 1 - p->pickup_transit)) {
                            continue;
                        }
                        if(pu == PortType::DELIVERY && (!created_de[p] || t > p->delivery_transit)) {
                            continue;
                        }

                        for(auto q : data.ports) {
                            if(p == q || p->models_same_port_as(*q)) {
                                continue;
                            }

                            if(q->hub) {
                                continue;
                            }

                            if(!q->allowed[vessel_class]) {
                                continue;
                            }

                            for(const auto &sc : vessel_class->bunker_cost_per_time_unit) {
                                auto distance = data.distances.at(std::make_pair(p, q));
                                auto speed = sc.first;
                                auto bunker_cost_per_time_unit = sc.second;
                                auto arrival_time = t + (int) ceil(distance / speed);

                                if(arrival_time >= data.num_times) {
                                    continue;
                                }

                                if(arrival_time < earliest_arrival(data, p, n_h1.port, *vessel_class)) {
                                    continue;
                                }

                                auto bunker_cost = (arrival_time - t) * bunker_cost_per_time_unit;
                                auto fixed_port_fee = q->fixed_fee;
                                auto variable_port_fee = q->variable_fee[vessel_class];

                                if((pu == PortType::DELIVERY) ||
                                   (pu == PortType::PICKUP &&
                                    p->pickup_demand + q->pickup_demand <= vessel_class->capacity)) {

                                    if(created_pu[q]) {
                                        auto final_time_pu = final_time(data, *q, arrival_time, PortType::PICKUP);

                                        if((final_time_pu <= latest_departure(data, q, n_h2.port, *vessel_class)) &&
                                           (final_time_pu >= data.num_times - 1 - q->pickup_transit)) {

                                            auto time_charter_cost = (final_time_pu - t) * vessel_class->time_charter_cost_per_time_unit;
                                            auto hotel_cost = (final_time_pu - arrival_time) * vessel_class->hotel_cost_per_time_unit;
                                            auto movement_cost = q->pickup_movement_cost;
                                            auto revenue = q->pickup_revenue;

                                            create_edge(*p, pu, t, *q, PortType::PICKUP, final_time_pu, g,
                                                        bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                                        fixed_port_fee + variable_port_fee, revenue, distance,
                                                        speed, ArcType::PORT_TO_PORT);
                                        }
                                    }
                                }

                                if((pu == PortType::DELIVERY &&
                                    p->delivery_demand + q->delivery_demand <= vessel_class->capacity) ||
                                   (pu == PortType::PICKUP &&
                                    p->pickup_demand + q->delivery_demand <= vessel_class->capacity)) {

                                    if(created_de[q]) {
                                        auto final_time_de = final_time(data, *q, arrival_time, PortType::DELIVERY);

                                        if((final_time_de <= latest_departure(data, q, n_h2.port, *vessel_class)) &&
                                           (final_time_de <= q->delivery_transit)) {

                                            auto time_charter_cost = (final_time_de - t) * vessel_class->time_charter_cost_per_time_unit;
                                            auto hotel_cost = (final_time_de - arrival_time) * vessel_class->hotel_cost_per_time_unit;
                                            auto movement_cost = q->delivery_movement_cost;
                                            auto revenue = q->delivery_revenue;

                                            create_edge(*p, pu, t, *q, PortType::DELIVERY, final_time_de, g,
                                                        bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                                        fixed_port_fee + variable_port_fee, revenue, distance,
                                                        speed, ArcType::PORT_TO_PORT);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            /*  Add delivery-to-pickup edges */
            for(auto vp = vertices(g->graph); vp.first != vp.second; ++vp.first) {
                auto n = *g->graph[*vp.first];

                if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PortType::DELIVERY && created_pu[n.port] &&
                   created_de[n.port]) {
                    // 1) Create an arc from port delivery to port pickup
                    auto t = n.time_step;
                    auto arrival_time = n.time_step;
                    auto final_time_pu = final_time(data, *n.port, arrival_time, PortType::PICKUP);

                    if((final_time_pu <= latest_departure(data, n.port, n_h2.port, *vessel_class)) &&
                       (final_time_pu >= data.num_times - 1 - n.port->pickup_transit)) {

                        auto time_charter_cost = (final_time_pu - t) * vessel_class->time_charter_cost_per_time_unit;
                        auto hotel_cost = (final_time_pu - arrival_time) * vessel_class->hotel_cost_per_time_unit;
                        auto bunker_cost = 0;
                        auto movement_cost = n.port->pickup_movement_cost;
                        auto fixed_port_fee = 0;
                        auto variable_port_fee = 0;
                        auto revenue = n.port->pickup_revenue;

                        create_edge(*n.port, n.pu_type, t, *n.port, PortType::PICKUP, final_time_pu, g,
                                    bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                    fixed_port_fee + variable_port_fee, revenue, 0.0,
                                    0.0, ArcType::DELIVERY_TO_PICKUP);
                    }
                }

                if(n.n_type == NodeType::REGULAR_PORT) {
                    // 2) Create arcs from port delivery to other ports (modelling the same physical location) both for delivery and pickup
                    for(auto vp2 = vertices(g->graph); vp2.first != vp2.second; ++vp2.first) {
                        auto n2 = *g->graph[*vp2.first];

                        if(n2.n_type == NodeType::REGULAR_PORT && // Is regular port
                           n.port != n2.port && // Is not exactely the same port as te first one [taken care in case 1]
                           n.port->models_same_port_as(*n2.port) // Models the same physical port as the first one
                            ) {
                            auto t = n.time_step;
                            auto arrival_time = n.time_step;

                            if(n2.pu_type == PortType::PICKUP) {
                                auto final_time_pu = final_time(data, *n2.port, arrival_time, PortType::PICKUP);

                                if((final_time_pu <= latest_departure(data, n2.port, n_h2.port, *vessel_class)) &&
                                   (final_time_pu >= data.num_times - 1 - n2.port->pickup_transit)) {

                                    auto time_charter_cost = (final_time_pu - t) * vessel_class->time_charter_cost_per_time_unit;
                                    auto hotel_cost = (final_time_pu - arrival_time) * vessel_class->hotel_cost_per_time_unit;
                                    auto bunker_cost = 0;
                                    auto movement_cost = n2.port->pickup_movement_cost;
                                    auto fixed_port_fee = 0;
                                    auto variable_port_fee = 0;
                                    auto revenue = n2.port->pickup_revenue;

                                    create_edge(*n.port, n.pu_type, t, *n2.port, PortType::PICKUP, final_time_pu, g,
                                                bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                                fixed_port_fee + variable_port_fee, revenue, 0.0,
                                                0.0, ArcType::DELIVERY_TO_PICKUP);
                                }
                            }

                            if(n2.pu_type == PortType::DELIVERY) {
                                auto final_time_de = final_time(data, *n2.port, arrival_time, PortType::DELIVERY);

                                if((final_time_de <= latest_departure(data, n2.port, n_h2.port, *vessel_class)) &&
                                   (final_time_de <= n2.port->delivery_transit)) {

                                    auto time_charter_cost = (final_time_de - t) * vessel_class->time_charter_cost_per_time_unit;
                                    auto hotel_cost = (final_time_de - arrival_time) * vessel_class->hotel_cost_per_time_unit;
                                    auto bunker_cost = 0;
                                    auto movement_cost = n2.port->delivery_movement_cost;
                                    auto fixed_port_fee = 0;
                                    auto variable_port_fee = 0;
                                    auto revenue = n2.port->delivery_revenue;

                                    create_edge(*n.port, n.pu_type, t, *n2.port, PortType::DELIVERY, final_time_de, g,
                                                bunker_cost + hotel_cost, time_charter_cost, movement_cost,
                                                fixed_port_fee + variable_port_fee, revenue, 0.0,
                                                0.0, ArcType::PORT_TO_PORT);
                                }
                            }
                        }
                    }
                }
            }

            /*  Do some spring cleaning */
            auto clean = false;
            while(!clean) {
                clean = true;
                vit vi, vi_end, vi_next;
                std::tie(vi, vi_end) = vertices(g->graph);
                for(vi_next = vi; vi != vi_end; vi = vi_next) {
                    ++vi_next;
                    auto n_out = (int) out_degree(*vi, g->graph);
                    auto n_in = (int) in_degree(*vi, g->graph);
                    if((g->graph[*vi]->n_type == NodeType::REGULAR_PORT) &&
                       (n_out == 0 || n_in == 0 || n_out + n_in <= 1)) {
                        clear_vertex(*vi, g->graph);
                        remove_vertex(*vi, g->graph);
                        clean = false;
                    }
                }
            }

            vit vi, vi_end;
            for(std::tie(vi, vi_end) = vertices(g->graph); vi != vi_end; ++vi) {
                oeit ei, ei_end, ei_next;
                std::tie(ei, ei_end) = out_edges(*vi, g->graph);

                std::unordered_map<Vertex, Edge> best_edges;
                for(ei_next = ei; ei != ei_end; ei = ei_next) {
                    ++ei_next;

                    auto destination = target(*ei, g->graph);
                    auto cost = g->graph[*ei]->cost;

                    if(best_edges.find(destination) == best_edges.end()) {
                        best_edges.emplace(destination, *ei);
                    } else {
                        if(g->graph[best_edges[destination]]->cost > cost) {
                            auto old_edge = best_edges[destination];
                            best_edges[destination] = *ei;
                            remove_edge(old_edge, g->graph);
                        } else {
                            remove_edge(*ei, g->graph);
                        }
                    }
                }
            }

            g->prepare_for_labelling();

            return g;
        }

        int final_time(const ProblemData &data, const Port &p, int arrival_time, PortType pu) {
            auto ft_handling = arrival_time + (pu == PortType::PICKUP ? p.pickup_handling : p.delivery_handling);
            auto ft = ft_handling;
            auto free_from_tw = false;

            while(!free_from_tw) {
                free_from_tw = true;
                for(const auto &tw : p.closing_time_windows) {
                    auto t1 = tw.first, t2 = tw.second;
                    if(ft <= t2 && ft > t1) {
                        ft += (t2 - t1);
                        free_from_tw = false;
                    }
                }
            }

            return std::min(ft, data.num_times - 1);
        }

        int latest_departure(const ProblemData &data, std::shared_ptr<Port> p, std::shared_ptr<Port> h2,
                             const VesselClass &vessel_class) {
            auto top_speed = vessel_class.top_speed;
            return std::max(0.0, data.num_times - floor(data.distances.at(std::make_pair(p, h2)) / top_speed));
        }

        int earliest_arrival(const ProblemData &data, std::shared_ptr<Port> p, std::shared_ptr<Port> h1,
                             const VesselClass &vessel_class) {
            auto top_speed = vessel_class.top_speed;
            return std::min(data.num_times - 1.0, ceil(data.distances.at(std::make_pair(h1, p)) / top_speed));
        }

        void create_edge(const Port &origin_p, PortType origin_pu, int origin_time,
                         const Port &destination_p, PortType destination_pu, int destination_time,
                         std::shared_ptr<Graph> g, double bunker_costs, double tc_costs,
                         double movement_costs, double port_costs, double revenue, double length,
                         double speed, ArcType type) {
            bool origin_found, destination_found;
            Vertex origin_v, destination_v;

            std::tie(origin_found, origin_v) = g->get_vertex(origin_p, origin_pu, origin_time);

            if(!origin_found) {
                return;
            }

            std::tie(destination_found, destination_v) = g->get_vertex(destination_p, destination_pu, destination_time);

            if(!destination_found) {
                return;
            }

            Edge e = add_edge(origin_v, destination_v, g->graph).first;
            g->graph[e] = std::make_shared<Arc>(bunker_costs, tc_costs, movement_costs, port_costs, revenue, length, speed, type);
        }
    }
}