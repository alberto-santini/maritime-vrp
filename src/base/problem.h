//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef PROBLEM_H
#define PROBLEM_H

#include <memory>
#include <string>
#include <unordered_map>

#include "graph.h"
#include "vessel_class.h"
#include "../preprocessing/problem_data.h"
#include "../preprocessing/program_params.h"

namespace mvrp {
    using GraphMap = std::unordered_map<std::shared_ptr<VesselClass>, std::shared_ptr<Graph>>;
    using ErasedEdgesMap = std::unordered_map<std::shared_ptr<VesselClass>, ErasedEdges>;

    struct Problem {
        /**
         * Algorithm parameters.
         */
        ProgramParams params;

        /**
         * Problem data.
         */
        ProblemData data;

        /**
         * List of graphs, one for each vessel class.
         */
        GraphMap graphs;

        Problem(const std::string &params_file, const std::string &data_file);
    };
}

#endif