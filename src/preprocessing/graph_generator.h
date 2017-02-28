//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef GRAPH_GENERATOR_H
#define GRAPH_GENERATOR_H

#include <memory>

#include "../base/graph.h"
#include "../base/node.h"
#include "../base/port.h"
#include "../base/vessel_class.h"
#include "problem_data.h"
#include "program_params.h"

namespace mvrp {
    namespace GraphGenerator {
        std::shared_ptr<Graph>
        create_graph(const ProblemData &data, const ProgramParams &params, std::shared_ptr<VesselClass> vessel_class);

        /*  Time at which a vessel arriving at port ("p","pu") at time "arrival_time" can leave that port */
        int final_time(const ProblemData &data, const Port &p, int arrival_time, PortType pu);

        /*  Latest time at which a vessel of class "vessel_class" can leave port "p" to arrive at H2
            before the end of the planning horizon, supposing it travels as fast as possible */
        int latest_departure(const ProblemData &data, std::shared_ptr<Port> p, std::shared_ptr<Port> h2,
                             const VesselClass &vessel_class);

        /*  Analogous to the previous one, but its "dual" */
        int earliest_arrival(const ProblemData &data, std::shared_ptr<Port> p, std::shared_ptr<Port> h1,
                             const VesselClass &vessel_class);

        void create_edge(const Port &origin_p, PortType origin_pu, int origin_time,
                         const Port &destination_p, PortType destination_pu, int destination_time,
                         std::shared_ptr<Graph> g, double bunker_costs, double tc_costs,
                         double movement_costs, double port_costs, double revenue, double length,
                         double speed, ArcType type);
    }
}

#endif
