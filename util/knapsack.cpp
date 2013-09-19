//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <util/knapsack.h>

int Knapsack::solve(const vector<int> demands, const int capacity) {
    vector<int> vals = demands;
    int used_capacity = 0;
    int visited_ports = 0;
    
    while(vals.size() > 0) {
        auto b_it = min_element(vals.begin(), vals.end());
        int best = *b_it;
        
        if(used_capacity + best <= capacity) {
            vals.erase(b_it);
            used_capacity += best;
            visited_ports++;
        } else break;
    }
    
    return visited_ports;
}