//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>

#include <base/vessel_class.h>

VesselClass::VesselClass(const std::string& name, int capacity, int num_vessels, double base_cost, const SpeedCostMap& bunker_cost) : name(name), capacity(capacity), num_vessels(num_vessels), base_cost(base_cost), bunker_cost(bunker_cost) {
    top_speed = std::max_element(bunker_cost.begin(), bunker_cost.end(), [] (const auto& sc1, const auto& sc2) { return (sc1.first < sc2.first); })->first;
}