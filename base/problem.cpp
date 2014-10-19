//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <iostream>

#include <base/problem.h>
#include <preprocessing/graph_generator.h>

Problem::Problem(const std::string& params_file, const std::string& data_file) : params(params_file), data(data_file), graphs() {
    for(auto vessel_class : data.vessel_classes) {
        auto g = GraphGenerator::create_graph(data, vessel_class, params.remove_additional_arcs);
        std::cerr << "Graph for " << vessel_class->name << ": " << num_vertices(g->graph) << " vertices and " << num_edges(g->graph) << " edges" << std::endl;
        graphs.emplace(vessel_class, g);
    }
}