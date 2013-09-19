//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef GRAPH_GENERATOR_H
#define GRAPH_GENERATOR_H

#include <base/base.h>
#include <base/graph.h>
#include <base/node.h>
#include <base/vessel_class.h>
#include <preprocessing/problem_data.h>

namespace GraphGenerator {
    void create_graph(const ProblemData& data, std::shared_ptr<VesselClass> vessel_class, Graph& g);
    
    /*  Time at which a vessel arriving at port ("p","pu") at time "arrival_time" can leave that port */
    int final_time(const ProblemData& data, std::shared_ptr<Port> p, const int arrival_time, const PickupType pu);
    
    /*  Latest time at which a vessel of class "vessel_class" can leave port "p" to arrive at H2
        before the end of the planning horizon, supposing it travels as fast as possible */
    int latest_departure(const ProblemData& data, std::shared_ptr<Port> p, std::shared_ptr<Port> h2, std::shared_ptr<VesselClass> vessel_class);
    
    /*  Analogous to the previous one, but its "dual" */
    int earliest_arrival(const ProblemData& data, std::shared_ptr<Port> p, std::shared_ptr<Port> h1, std::shared_ptr<VesselClass> vessel_class);
    
    void create_edge(std::shared_ptr<Port> origin_p, const PickupType origin_pu, const int origin_t,
                     std::shared_ptr<Port> destination_p, const PickupType destination_pu, const int destination_t,
                     Graph& g, const float cost);
}

#endif
