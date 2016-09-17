//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef ARC_H
#define ARC_H

namespace mvrp {
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
         * Unique progressive id used by boost algorithms.
         */
        int boost_edge_id;

        Arc() {}

        Arc(double bunker_costs, double tc_costs, double movement_costs, double port_costs, double revenue,
            double length) :
            cost(bunker_costs + tc_costs + movement_costs + port_costs - revenue), bunker_costs(bunker_costs),
            tc_costs(tc_costs), movement_costs(movement_costs), port_costs(port_costs), revenue(revenue),
            length(length) {
            boost_edge_id = 0;
        }
    };
}

#endif
