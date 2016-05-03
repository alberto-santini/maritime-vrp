//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <limits>
#include <iostream>

#include <column/solution.h>

bool Solution::satisfies_capacity_constraints() const {
    auto qty_delivered = 0;
    
    for(auto pit = path.rbegin(); pit != path.rend(); ++pit) {
        const Node& n = *g->graph[target(*pit, g->graph)];
        if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PickupType::DELIVERY) {
            qty_delivered += n.de_demand();
        }
    }
    
    if(qty_delivered > vessel_class->capacity) {
        return false;
    }
    
    auto used_capacity = qty_delivered;
    
    for(auto pit = path.rbegin(); pit != path.rend(); ++pit) {
        const Node& n = *g->graph[target(*pit, g->graph)];
        
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

bool Solution::operator==(const Solution& other) const {
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
    
    for(const auto& e : path) {
        l += g->graph[e]->length;
    }
    
    return l;
}

std::vector<double> Solution::cargo_travel_distances() const {
    auto l = length();
    auto current_distance = 0.0;
    auto distances = std::vector<double>();
    
    for(const auto & e : path) {
        current_distance += g->graph[e]->length;
        
        const Node& dest = *g->graph[target(e, g->graph)];
        
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
    
    for(const auto& e : path) {
        dist.push_back(g->graph[e]->length);
    }
    
    return dist;
}

std::vector<double> Solution::legs_speed() const {
    auto speeds = std::vector<double>();
    
    for(const auto& e : path) {
        auto s = source(e, g->graph);
        auto t = target(e, g->graph);
        auto l = g->graph[e]->length;
        auto s_ti = g->graph[s]->time_step;
        auto t_ti = g->graph[t]->time_step;
        speeds.push_back(l / (t_ti - s_ti));
    }
    
    return speeds;
}