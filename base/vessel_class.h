//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef VESSEL_CLASS_H
#define VESSEL_CLASS_H

#include <string>
#include <unordered_map>

typedef std::unordered_map<float, float> SpeedCostMap;

class VesselClass {
public:
    std::string     name;
    int             capacity;
    int             num_vessels;
    float           base_cost;
    SpeedCostMap    bunker_cost;
    float           top_speed;
    
    VesselClass(const std::string& name, int capacity, int num_vessels, float base_cost, const SpeedCostMap& bunker_cost);
};

#endif
