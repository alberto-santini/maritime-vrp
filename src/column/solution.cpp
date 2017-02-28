//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <limits>
#include <iostream>

#include "solution.h"

namespace mvrp {
    bool Solution::satisfies_capacity_constraints() const {
        auto current_start = path.rbegin();

        while(current_start != path.rend()) {
            auto qty_delivered = 0;
            auto next_start = path.rend();

            for(auto pit = current_start; pit != path.rend(); ++pit) {
                const Node &n = *g->graph[target(*pit, g->graph)];

                if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PortType::DELIVERY) {
                    qty_delivered += n.de_demand();
                    if(qty_delivered > vessel_class->capacity) { return false; }
                }

                if(n.n_type == NodeType::COMEBACK_HUB) {
                    next_start = current_start + 1;
                    break;
                }
            }

            auto used_capacity = qty_delivered;

            for(auto pit = current_start; pit != next_start; ++pit) {
                const Node &n = *g->graph[target(*pit, g->graph)];

                if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PortType::DELIVERY) {
                    used_capacity -= n.de_demand();
                }

                if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PortType::PICKUP) {
                    if(used_capacity + n.pu_demand() > vessel_class->capacity) {
                        return false;
                    }
                    used_capacity += n.pu_demand();
                }
            }

            current_start = next_start;
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

            if(p.pu_type == PortType::PICKUP) {
                current += p.pu_demand();
            } else if(p.pu_type == PortType::DELIVERY) {
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
                if(dest.pu_type == PortType::PICKUP) {
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
            auto h = g->graph[t]->handling_time();

            speeds.push_back(l / (t_ti - h - s_ti));
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

    bool Solution::uses_arc(Edge e) const {
        return std::find(path.begin(), path.end(), e) != path.end();
    }

    bool Solution::visits_port(const Port& port, const PortType& pu_type) const {
        for(const auto& edge : path) {
            auto v = boost::source(edge, g->graph);
            const auto& n = g->graph[v];

            if(*(n->port) == port && n->pu_type == pu_type) { return true; }
        }
        return false;
    }

    bool Solution::visits_consecutive_ports(const PortWithType& pred, const PortWithType& succ) const {
        for(const auto& edge : path) {
            auto srcv = boost::source(edge, g->graph);
            auto trgv = boost::target(edge, g->graph);
            const auto& srcn = g->graph[srcv];
            const auto& trgn = g->graph[trgv];

            if(*(srcn->port) == *(pred.first) && *(trgn->port) == *(succ.first) && srcn->pu_type == pred.second && trgn->pu_type == succ.second) { return true; }
        }
        return false;
    }

    bool Solution::visits_consecutive_ports_at_speed(const PortWithType& pred, const PortWithType& succ, double speed) const {
        for(const auto& edge : path) {
            auto srcv = boost::source(edge, g->graph);
            auto trgv = boost::target(edge, g->graph);
            const auto& srcn = g->graph[srcv];
            const auto& trgn = g->graph[trgv];

            if( *(srcn->port) == *(pred.first) &&
                *(trgn->port) == *(succ.first) &&
                srcn->pu_type == pred.second &&
                trgn->pu_type == succ.second &&
                std::abs(g->graph[edge]->speed - speed) < 1e-6
            ) { return true; }
        }
        return false;
    }

    Solution::PortsWithPredecessors Solution::visited_ports_with_predecessors() const {
        PortsWithPredecessors visited;

        for(const auto& edge : path) {
            auto srcv = boost::source(edge, g->graph);
            auto trgv = boost::target(edge, g->graph);
            const auto& srcn = g->graph[srcv];
            const auto& trgn = g->graph[trgv];

            if(trgn->n_type == NodeType::REGULAR_PORT) {
                auto key = std::make_pair(trgn->port.get(), trgn->pu_type);
                auto val = std::make_pair(srcn->port.get(), srcn->pu_type);

                if(visited.find(key) == visited.end()) {
                    visited[key] = std::vector<PortWithType>{};
                }

                visited[key].push_back(val);
            }
        }

        return visited;
    };

    boost::optional<std::pair<PortWithType, PortWithType>> Solution::common_port_visited_from_two_different_predecessors(const Solution& other) const {
        auto this_ports = visited_ports_with_predecessors();
        auto other_ports = other.visited_ports_with_predecessors();

        for(const auto& kv : this_ports) {
            if(other_ports.find(kv.first) == other_ports.end()) { continue; }
            const auto& successor = kv.first;
            const auto& this_predecessors = kv.second;
            const auto& other_predecessors = other_ports[successor];

            for(const auto& p : this_predecessors) {
                if(std::find(other_predecessors.begin(), other_predecessors.end(), p) == other_predecessors.end()) {
                    return std::make_pair(p, successor);
                }
            }
        }

        return boost::none;
    };

    boost::optional<std::tuple<PortWithType, PortWithType, double>> Solution::common_port_succession_at_two_different_speeds(const Solution& other) const {
        for(const auto& edge : path) {
            auto srcv = boost::source(edge, g->graph);
            auto trgv = boost::target(edge, g->graph);
            auto speed = g->graph[edge]->speed;
            const auto& srcn = g->graph[srcv];
            const auto& trgn = g->graph[trgv];

            auto src = std::make_pair(srcn->port.get(), srcn->pu_type);
            auto trg = std::make_pair(trgn->port.get(), trgn->pu_type);

            if(
                std::any_of(
                    other.vessel_class->bunker_cost_per_time_unit.begin(),
                    other.vessel_class->bunker_cost_per_time_unit.end(),
                    [&] (const auto& speed_cost) -> bool {
                        return other.visits_consecutive_ports_at_speed(src, trg, speed_cost.first);
                    }
                )
            ) {
                return std::make_tuple(src, trg, speed);
            }
        }

        return boost::none;
    };
}
