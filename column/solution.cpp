//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <column/solution.h>

bool Solution::satisfies_capacity_constraints(const Graph& g) {
    int qty_delivered = 0;
    
    Path::const_reverse_iterator pit;
    for(pit = path.rbegin(); pit != path.rend(); ++pit) {
        Node n = *g.graph[target(*pit, g.graph)];
        if(n.n_type == NodeType::REGULAR_PORT && n.pu_type == PickupType::DELIVERY) {
            qty_delivered += n.de_demand();
        }
    }
    
    if(qty_delivered > vessel_class->capacity) {
        return false;
    }
    
    int used_capacity = qty_delivered;
    
    for(pit = path.rbegin(); pit != path.rend(); ++pit) {
        Node n = *g.graph[target(*pit, g.graph)];
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