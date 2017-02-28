//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef ARC_H
#define ARC_H

#include <iostream>

namespace mvrp {
    enum class ArcType {
        SOURCE_TO_PORT, PORT_TO_SINK, COMEBACK_HUB_TO_PORT, PORT_TO_COMEBACK_HUB, PORT_TO_PORT, DELIVERY_TO_PICKUP
    };

    struct Arc {
        /**
         * Total cost of the arc.
         */
        double cost;

        /**
         * Bunker cost when traversing the arc.
         */
        double bunker_costs;

        /**
         * Time charter cost.
         */
        double tc_costs;

        /**
         * Container movement cost at the destination port.
         */
        double movement_costs;

        /**
         * Port fee at the destination port.
         */
        double port_costs;

        /**
         * Revenue earned at the destination port.
         */
        double revenue;

        /**
         * Length of the trip in nautical miles.
         */
        double length;

        /**
         * Vessel speed along the arc.
         */
        double speed;

        /**
         * Unique progressive id used by boost algorithms.
         */
        int boost_edge_id;

        /**
         * Arc type
         */
        ArcType type;

        Arc() {}

        Arc(double bunker_costs, double tc_costs, double movement_costs, double port_costs, double revenue,
            double length, double speed, ArcType type) :
            cost(bunker_costs + tc_costs + movement_costs + port_costs - revenue), bunker_costs(bunker_costs),
            tc_costs(tc_costs), movement_costs(movement_costs), port_costs(port_costs), revenue(revenue),
            length(length), speed(speed), type(type) {
            boost_edge_id = 0;
        }
    };

    std::ostream& operator<<(std::ostream& out, const ArcType& arctype);
}

#endif
