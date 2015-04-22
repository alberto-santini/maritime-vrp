//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>

#include <util/knapsack.h>

int Knapsack::solve(const std::vector<int>& demands, int capacity) {
    auto vals = demands;
    auto used_capacity = 0;
    auto visited_ports = 0;
    
    while(vals.size() > 0) {
        auto b_it = std::min_element(vals.begin(), vals.end());
        auto best = *b_it;
        
        if(used_capacity + best <= capacity) {
            vals.erase(b_it);
            used_capacity += best;
            visited_ports++;
        } else break;
    }
    
    return visited_ports;
}