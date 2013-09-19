//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef PROBLEM_H
#define PROBLEM_H

#include <base/base.h>
#include <base/graph.h>
#include <base/vessel_class.h>
#include <preprocessing/graph_generator.h>
#include <preprocessing/problem_data.h>
#include <preprocessing/program_params.h>

typedef std::unordered_map<std::shared_ptr<VesselClass>, Graph> GraphMap;

class Problem {
public:
    ProgramParams    params;
    ProblemData      data;
    GraphMap         graphs;
    
    Problem(string params_file = "data/program_params.json", string data_file = "data/problem_data.json");
};

#endif