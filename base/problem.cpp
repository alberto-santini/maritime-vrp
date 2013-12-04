//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <base/problem.h>

Problem::Problem(string params_file, string data_file) {
    params = ProgramParams(params_file);
    data = ProblemData(data_file);
    graphs = GraphMap();
    
    for(std::shared_ptr<VesselClass> vessel_class : data.vessel_classes) {
        std::shared_ptr<Graph> g = GraphGenerator::create_graph(data, vessel_class, params.remove_additional_arcs);
        cerr << "Graph for " << vessel_class->name << ": " << num_vertices(g->graph) << " vertices and " << num_edges(g->graph) << " edges" << endl;
        graphs.emplace(vessel_class, g);
    }
}