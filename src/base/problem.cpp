//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <iostream>

#include "problem.h"
#include "../preprocessing/graph_generator.h"

namespace mvrp {
    Problem::Problem(const std::string &params_file, const std::string &data_file) : params{params_file}, data{data_file}, graphs{} {
        for(auto vessel_class : data.vessel_classes) {
            auto g = GraphGenerator::create_graph(data, params, vessel_class);

            std::cerr << "Graph for " << vessel_class->name << ": " << boost::num_vertices(g->graph);
            std::cerr << " vertices and " << boost::num_edges(g->graph) << " edges" << std::endl;

            graphs.emplace(vessel_class, g);
        }
    }
}
