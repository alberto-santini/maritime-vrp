//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <limits>
#include <iostream>

#include "solution.h"

namespace mvrp {
    bool Solution::satisfies_capacity_constraints() const {
        auto qty_delivered = 0;

        for(auto pit = path.rbegin(); pit != path.rend(); ++pit) {
            const Node &n = *g->graph[target(*pit, g->graph)];
            if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PickupType::DELIVERY) {
                qty_delivered += n.de_demand();
            }
        }

        if(qty_delivered > vessel_class->capacity) {
            return false;
        }

        auto used_capacity = qty_delivered;

        for(auto pit = path.rbegin(); pit != path.rend(); ++pit) {
            const Node &n = *g->graph[target(*pit, g->graph)];

            if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PickupType::DELIVERY) {
                used_capacity -= n.de_demand();
            }
            if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PickupType::PICKUP) {
                if(used_capacity + n.pu_demand() > vessel_class->capacity) {
                    return false;
                }
                used_capacity += n.pu_demand();
            }
        }

        return true;
    }

    bool Solution::operator==(const Solution &other) const {
        if(other.vessel_class != vessel_class) {
            return false;
        }
        if(other.path.size() != path.size()) {
            return false;
        }
        if(fabs(cost - other.cost) > 0.000001) {
            return false;
        }
        for(auto i = 0u; i < path.size(); i++) {
            if(*other.g->graph[source(other.path[i], other.g->graph)] != *g->graph[source(path[i], g->graph)]) {
                return false;
            }
        }
        return true;
    }

    double Solution::length() const {
        auto l = 0.0;

        for(const auto &e : path) {
            l += g->graph[e]->length;
        }

        return l;
    }

    double Solution::highest_load_efficiency() const {
        auto highest = 0.0;
        auto current = 0.0;

        for(const auto &e : path) {
            auto v = target(e, g->graph);
            const auto& p = *g->graph[v];

            if(p.pu_type == PickupType::PICKUP) {
                current += p.pu_demand();
            } else if(p.pu_type == PickupType::DELIVERY) {
                current -= p.de_demand();
            }

            if(current > highest) { highest = current; }
        }

        return highest / vessel_class->capacity;
    }

    std::vector<double> Solution::cargo_travel_distances() const {
        auto l = length();
        auto current_distance = 0.0;
        auto distances = std::vector<double>();

        for(const auto &e : path) {
            current_distance += g->graph[e]->length;

            const Node &dest = *g->graph[target(e, g->graph)];

            if(dest.n_type == NodeType::REGULAR_PORT) {
                if(dest.pu_type == PickupType::PICKUP) {
                    distances.push_back(l - current_distance);
                } else {
                    distances.push_back(current_distance);
                }
            }
        }

        assert(std::abs(current_distance - l) < 0.00001);

        return distances;
    }

    std::vector<double> Solution::legs_distance() const {
        auto dist = std::vector<double>();

        for(const auto &e : path) {
            dist.push_back(g->graph[e]->length);
        }

        return dist;
    }

    uint32_t Solution::n_ports_visited() const {
        auto n_ports = 0u;

        for(const auto& e : path) {
            auto v = target(e, g->graph);

            if(g->graph[v]->n_type == NodeType::REGULAR_PORT) {
                n_ports++;
            }
        }

        return n_ports;
    }

    std::vector<double> Solution::legs_speed() const {
        auto speeds = std::vector<double>();

        for(const auto &e : path) {
            auto s = source(e, g->graph);
            auto t = target(e, g->graph);
            auto l = g->graph[e]->length;
            auto s_ti = g->graph[s]->time_step;
            auto t_ti = g->graph[t]->time_step;
            speeds.push_back(l / (t_ti - s_ti));
        }

        return speeds;
    }

    SolutionCosts Solution::solution_costs() const {
        SolutionCosts s;

        for(const auto &e : path) {
            s.add(SolutionCosts(
                g->graph[e]->bunker_costs,
                g->graph[e]->tc_costs,
                g->graph[e]->port_costs,
                g->graph[e]->movement_costs,
                g->graph[e]->revenue
            ));
        }

        return s;
    }
}