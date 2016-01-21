//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <limits>

#include <branching/cycle.h>

Path Cycle::shortest_cycle(const Path& route, std::shared_ptr<const Graph> g) {
    if(route.size() < 2) {
        return Path();
    }
    
    std::vector<Path> cycles;
    auto num_cycles = 0;
    auto shortest_cycle_length = std::numeric_limits<unsigned int>::max();
    auto shortest_cycle_idx = 0;
    
    cycles.push_back(Path());
    
    for(auto i = route.size() - 1; i > 0; i--) {
        auto closing_cycle = false;
        auto closing_port_position = -1;
        
        const Node& current_node = *g->graph[target(route[i], g->graph)];
        auto current_port = current_node.port;
        const PickupType& current_pu = current_node.pu_type;
        
        for(auto k = 0u; k < cycles[num_cycles].size(); k++) {
            const Node& cycle_node = *g->graph[source(cycles[num_cycles][k], g->graph)];
            auto cycle_port = cycle_node.port;
            const PickupType& cycle_pu = cycle_node.pu_type;
            
            if(current_port == cycle_port && current_pu == cycle_pu) {
                cycles[num_cycles].push_back(route[i]);
                closing_cycle = true;
                closing_port_position = k;
                
                auto length = cycles[num_cycles].size() - k;
                if(length < shortest_cycle_length) {
                    shortest_cycle_length = length;
                    shortest_cycle_idx = num_cycles;
                }
                
                num_cycles++;
                cycles.push_back(Path());
                break;
            }
        }
        
        if(closing_cycle) {
            cycles[num_cycles - 1].erase(cycles[num_cycles - 1].begin(), cycles[num_cycles - 1].begin() + closing_port_position);
        } else {
            cycles[num_cycles].push_back(route[i]);
        }
    }
    
    // reverse(cycles[shortest_cycle_idx].begin(), cycles[shortest_cycle_idx].end());
    
    if(num_cycles == 0) {
        return Path();
    } else {
        return cycles[shortest_cycle_idx];
    }
}

void Cycle::print_cycle(const Path& cycle, std::shared_ptr<const Graph> g, std::ostream& out) {
    for(const auto& e : cycle) {
        const Node& n = *g->graph[source(e, g->graph)];
        out << n.port->name << "(" << (n.pu_type == PickupType::PICKUP ? "pu" : "de") << ") -> ";
    }
    
    const Node& n = *g->graph[target(cycle.back(), g->graph)];
    out << n.port->name << "(" << (n.pu_type == PickupType::PICKUP ? "pu" : "de") << ")" << std::endl;
}