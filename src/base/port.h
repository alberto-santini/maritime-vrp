//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef PORT_H
#define PORT_H

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "vessel_class.h"

namespace mvrp {
    using AllowedVcMap = std::unordered_map<std::shared_ptr<VesselClass>, bool>;
    using VcFee = std::unordered_map<std::shared_ptr<VesselClass>, double>;
    using ClosingTimeWindows = std::vector<std::pair<int, int>>;

    struct Port {
        /**
         * Port name (UNLO code).
         */
        std::string name;

        /**
         * Pickup demand in TEU.
         */
        int pickup_demand;

        /**
         * Delivery demand in TEU.
         */
        int delivery_demand;

        /**
         * Pickup transit time in time steps.
         */
        int pickup_transit;

        /**
         * Delivery transit time in time steps.
         */
        int delivery_transit;

        /**
         * Pickup handling time in time steps.
         */
        int pickup_handling;

        /**
         * Delivery handling time in time steps.
         */
        int delivery_handling;

        /**
         * True iff this port is the hub.
         */
        bool hub;

        /**
         * Map of Vessel Classes that can call the port.
         */
        AllowedVcMap allowed;

        /**
         * Cost to pickup 1 container at the port.
         */
        double pickup_movement_cost;

        /**
         * Cost to deliver 1 container at the port.
         */
        double delivery_movement_cost;

        /**
         * Fixed fee to pay when calling the port.
         */
        double fixed_fee;

        /**
         * Variable fee to pay when calling the port. It varies based on the Vessel Class.
         */
        VcFee variable_fee;

        /**
         * Revenue earned when doing pickup at the port.
         */
        double pickup_revenue;

        /**
         * Revenue earned when delivering at the port.
         */
        double delivery_revenue;

        /**
         * Peanlty to pay when skipping pickup service at the port.
         */
        double pickup_penalty;

        /**
         * Penalty to pay when skipping delivery service at the port.
         */
        double delivery_penalty;

        /**
         * Closing time-windows for the port.
         */
        ClosingTimeWindows closing_time_windows;

        Port() {}

        Port(const std::string &name, int pickup_demand, int delivery_demand, int pickup_transit, int delivery_transit,
             int pickup_handling, int delivery_handling, bool hub, const AllowedVcMap &allowed,
             double pickup_movement_cost,
             double delivery_movement_cost, double fixed_fee, VcFee variable_fee, double pickup_revenue,
             double delivery_revenue,
             double pickup_penalty, double delivery_penalty, const ClosingTimeWindows &closing_time_windows) :
             name(name), pickup_demand(pickup_demand), delivery_demand(delivery_demand), pickup_transit(pickup_transit),
             delivery_transit(delivery_transit), pickup_handling(pickup_handling), delivery_handling(delivery_handling),
             hub(hub), allowed(allowed), pickup_movement_cost(pickup_movement_cost),
             delivery_movement_cost(delivery_movement_cost),
             fixed_fee(fixed_fee), variable_fee(variable_fee), pickup_revenue(pickup_revenue),
             delivery_revenue(delivery_revenue),
             pickup_penalty(pickup_penalty), delivery_penalty(delivery_penalty),
             closing_time_windows(closing_time_windows) {}

        /**
         * Check if two port object model the same physical port. This is the case when their names start
         * with the same UNLO code.
         */
        inline bool models_same_port_as(const Port &q) const { return q.name.substr(0, 5) == name.substr(0, 5); }
    };
}
#endif
