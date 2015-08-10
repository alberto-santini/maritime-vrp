//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>

#include <base/vessel_class.h>

VesselClass::VesselClass(const std::string& name,
                         int capacity,
                         int num_vessels,
                         double time_charter_cost_per_time_unit,
                         double hotel_cost_per_time_unit,
                         const SpeedCostMap& bunker_cost_per_time_unit) :
        name(name),
        capacity(capacity),
        num_vessels(num_vessels),
        time_charter_cost_per_time_unit(time_charter_cost_per_time_unit),
        hotel_cost_per_time_unit(hotel_cost_per_time_unit),
        bunker_cost_per_time_unit(bunker_cost_per_time_unit)
{
    top_speed = std::max_element(bunker_cost_per_time_unit.begin(), bunker_cost_per_time_unit.end(), [] (const auto& sc1, const auto& sc2) { return (sc1.first < sc2.first); })->first;
}