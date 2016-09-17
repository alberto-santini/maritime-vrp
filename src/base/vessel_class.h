//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef VESSEL_CLASS_H
#define VESSEL_CLASS_H

#include <string>
#include <unordered_map>

namespace mvrp {
    using SpeedCostMap = std::unordered_map<double, double>;

    struct VesselClass {
        /**
         * Vessel class name.
         */
        std::string name;

        /**
         * Capacity in TEU.
         */
        int capacity;

        /**
         * Number of vessels available of this class.
         */
        int num_vessels;

        /**
         * Time charter cost per each time step.
         */
        double time_charter_cost_per_time_unit;

        /**
         * Hotel cost per each time step.
         */
        double hotel_cost_per_time_unit;

        /**
         * Bunker cost per each time step, at a certain speed.
         */
        SpeedCostMap bunker_cost_per_time_unit;

        /**
         * Maximum speed achievable.
         */
        double top_speed;

        VesselClass(const std::string &name, int capacity, int num_vessels, double time_charter_cost_per_time_unit,
                    double hotel_cost_per_time_unit, const SpeedCostMap& bunker_cost_per_time_unit);
    };
}

#endif