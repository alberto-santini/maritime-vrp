//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef SOLUTION_H
#define SOLUTION_H

#include <memory>

#include "../base/graph.h"
#include "../base/vessel_class.h"

namespace mvrp {
    struct SolutionCosts {
        double total_costs;
        double total_bunker;
        double total_time_charter;
        double total_ports;
        double total_movement;
        double total_revenue;

        SolutionCosts() : total_costs(0.0), total_bunker(0.0), total_time_charter(0.0), total_ports(0.0),
                          total_movement(0.0), total_revenue(0.0) {}

        SolutionCosts(double total_bunker, double total_time_charter, double total_ports, double total_movement, double total_revenue) :
            total_costs(2 * total_bunker + total_time_charter + total_ports + total_movement), total_bunker(2 * total_bunker),
            total_time_charter(total_time_charter), total_ports(total_ports), total_revenue(total_revenue) {}

        void add(const SolutionCosts &other) {
            total_costs += other.total_costs;
            total_bunker += other.total_bunker;
            total_time_charter += other.total_time_charter;
            total_ports += other.total_ports;
            total_movement += other.total_movement;
            total_revenue += other.total_revenue;
        }
    };

    struct Solution {
        // Key => Port
        // Value => List of ports visited immediately before <Key> in the current route (more than one in case of cycles)
        using PortsWithPredecessors = std::map<PortWithType, std::vector<PortWithType>>;

        Path path;
        double cost;
        double reduced_cost;
        std::shared_ptr<VesselClass> vessel_class;
        std::shared_ptr<const Graph> g;

        Solution() {}

        Solution(Path path, double reduced_cost, std::shared_ptr<const Graph> g) :
            path{path}, cost{g->calculate_path_cost(path)}, reduced_cost{reduced_cost}, vessel_class{g->vessel_class}, g{g} {}

        Solution(Path path, double cost, double reduced_cost, std::shared_ptr<VesselClass> vessel_class, std::shared_ptr<const Graph> g) :
            path{path}, cost{cost}, reduced_cost{reduced_cost}, vessel_class{vessel_class}, g{g} {}

        bool uses_arc(Edge e) const;
        bool visits_port(const Port& port, const PortType& pu_type) const;
        bool visits_port(const PortWithType& pt) const { return visits_port(*pt.first, pt.second); }
        bool visits_consecutive_ports(const PortWithType& pred, const PortWithType& succ) const;
        bool visits_consecutive_ports_at_speed(const PortWithType& pred, const PortWithType& succ, double speed) const;

        bool satisfies_capacity_constraints() const;

        PortsWithPredecessors visited_ports_with_predecessors() const;

        // If this route (r) and another route (s) have one port (i) in common and they visit i coming
        // from two different preceding ports (j,k) then returns the pair <j,i>.
        boost::optional<std::pair<PortWithType, PortWithType>> common_port_visited_from_two_different_predecessors(const Solution& other) const;

        // If this route (r) and another route (s) visited two ports (i,j) in succession
        // but have tdone so at different speeds (x,y) then returns the tuple <i,j,x>.
        boost::optional<std::tuple<PortWithType, PortWithType, double>> common_port_succession_at_two_different_speeds(const Solution& other) const;

        bool operator==(const Solution &other) const;
        double length() const;
        double highest_load_efficiency() const;
        uint32_t n_ports_visited() const;
        std::vector<double> cargo_travel_distances() const;
        std::vector<double> legs_distance() const;
        std::vector<double> legs_speed() const;
        SolutionCosts solution_costs() const;
    };
}
#endif