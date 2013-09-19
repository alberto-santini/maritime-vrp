//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef VESSEL_CLASS_H
#define VESSEL_CLASS_H

#include <base/base.h>

typedef std::unordered_map<float, float> SpeedCostMap;

class VesselClass {
public:
    string          name;
    int             capacity;
    int             num_vessels;
    float           base_cost;
    SpeedCostMap    bunker_cost;
    float           top_speed;
    
    VesselClass() {}
    VesselClass(const string name,
                const int capacity,
                const int num_vessels,
                const float base_cost,
                const SpeedCostMap bunker_cost) : name(name), capacity(capacity), num_vessels(num_vessels), base_cost(base_cost), bunker_cost(bunker_cost) {
        top_speed = max_element(bunker_cost.begin(),
                                bunker_cost.end(),
                                [] (const pair<float, float> sc1, const pair<float, float> sc2) {
                                    return (sc1.first < sc2.first);
                                })->first;
    }
};

#endif
