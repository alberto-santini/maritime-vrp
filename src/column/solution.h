//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef SOLUTION_H
#define SOLUTION_H

#include <memory>

#include <base/graph.h>
#include <base/vessel_class.h>

struct SolutionCosts {
    double total_costs;
    double total_bunker;
    double total_time_charter;
    double total_ports;
    double total_movement;
    double total_revenue;
    
    SolutionCosts()
    total_costs(0.0), total_bunker(0.0), total_time_charter(0.0), total_ports(0.0), total_movement(0.0), total_revenue(0.0) {}
    SolutionCosts(double total_bunker, double total_time_charter, double total_ports, double total_movement, double total_revenue) :
        total_costs(2 * total_bunker + total_time_charter + total_ports + total_movement),
        total_bunker(2 * total_bunker),
        total_time_charter(total_time_charter),
        total_ports(total_ports),
        total_revenue(total_revenue) {}
        
    void add(const SolutionCosts& other) {
        total_costs += other.total_costs;
        total_bunker += other.total_bunker;
        total_time_charter += other.total_time_charter;
        total_ports += other.total_ports;
        total_movement += other.total_movement;
        total_revenue += other.total_revenue;
    }
};

class Solution {
public:
    Path                            path;
    double                          cost;
    double                          reduced_cost;
    std::shared_ptr<VesselClass>    vessel_class;
    std::shared_ptr<const Graph>    g;
    
    Solution() {}
    
    Solution(Path path,
             double reduced_cost,
             std::shared_ptr<const Graph> g) :
             path{path},
             cost{g->calculate_cost(path)},
             reduced_cost{reduced_cost},
             vessel_class{g->vessel_class},
             g{g} {}
             
    Solution(Path path, 
             double cost,
             double reduced_cost,
             std::shared_ptr<VesselClass> vessel_class,
             std::shared_ptr<const Graph> g) :
             path{path},
             cost{cost},
             reduced_cost{reduced_cost},
             vessel_class{vessel_class},
             g{g} {}
    
    bool satisfies_capacity_constraints() const;
    bool operator==(const Solution& other) const;
    double length() const;
    std::vector<double> cargo_travel_distances() const;
    std::vector<double> legs_distance() const;
    std::vector<double> legs_speed() const;
    SolutionCosts solution_costs() const;
};

#endif