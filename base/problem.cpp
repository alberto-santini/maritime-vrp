//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <base/problem.h>

Problem::Problem(string params_file, string data_file) {
    params = ProgramParams(params_file);
    data = ProblemData(data_file);
    graphs = GraphMap();
    
    for(std::shared_ptr<VesselClass> vessel_class : data.vessel_classes) {
        Graph g;
        GraphGenerator::create_graph(data, vessel_class, g);
        graphs.emplace(vessel_class, g);
    }
}