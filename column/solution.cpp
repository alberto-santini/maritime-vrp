//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <limits>

#include <column/solution.h>

bool Solution::satisfies_capacity_constraints() const {
    auto qty_delivered = 0;
    
    for(auto pit = path.rbegin(); pit != path.rend(); ++pit) {
        auto n = *g->graph[target(*pit, g->graph)];
        if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PickupType::DELIVERY) {
            qty_delivered += n.de_demand();
        }
    }
    
    if(qty_delivered > vessel_class->capacity) {
        return false;
    }
    
    auto used_capacity = qty_delivered;
    
    for(auto pit = path.rbegin(); pit != path.rend(); ++pit) {
        auto n = *g->graph[target(*pit, g->graph)];
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
    if(fabs(cost - other.cost) > std::numeric_limits<float>::epsilon()) {
        return false;
    }
    for(int i = 0; i < path.size(); i++) {
        if(*other.g->graph[source(other.path[i], other.g->graph)] != *g->graph[source(path[i], g->graph)]) {
            return false;
        }
    }
    return true;
}