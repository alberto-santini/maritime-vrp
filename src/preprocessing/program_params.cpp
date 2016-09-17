//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "program_params.h"

namespace mvrp {
    ProgramParams::ProgramParams(const std::string &data_file_name) {
        using namespace boost::property_tree;

        ptree pt;
        read_json(data_file_name, pt);

        greedy_max_outarcs = pt.get<int>("greedy_max_outarcs");
        greedy_reruns = pt.get<int>("greedy_reruns");
        max_cols_to_solve_mp = pt.get<int>("max_cols_to_solve_mp");
        dummy_column_price = pt.get<long>("dummy_column_price");
        cplex_cores = pt.get<int>("cplex_cores");
        elementary_labelling_every_n_nodes = pt.get<int>("elementary_labelling_every_n_nodes");
        try_elementary_labelling = pt.get<bool>("try_elementary_labelling");
        try_smart_graph_reduction = pt.get<bool>("try_smart_graph_reduction");
        try_reduced_labelling = pt.get<bool>("try_reduced_labelling");
        smart_min_chance = pt.get<double>("smart_min_chance");
        smart_max_chance = pt.get<double>("smart_max_chance");
        time_limit_in_s = pt.get<int>("time_limit_in_s");
        parallel_labelling = pt.get<bool>("parallel_labelling");
        early_branching = pt.get<bool>("early_branching");
        early_branching_timeout = pt.get<int>("early_branching_timeout");
    }
}