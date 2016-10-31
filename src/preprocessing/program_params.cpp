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

        parallel_labelling = pt.get<bool>("parallel_labelling");
        early_branching = pt.get<bool>("early_branching");
        enable_cost_prize_acceleration = pt.get<bool>("enable_cost_prize_acceleration");
        enable_prize_acceleration = pt.get<bool>("enable_prize_acceleration");
        greedy_max_outarcs = pt.get<int>("greedy_max_outarcs");
        greedy_reruns = pt.get<int>("greedy_reruns");
        elementary_labelling_every_n_nodes = pt.get<int>("elementary_labelling_every_n_nodes");
        max_cols_to_solve_mp = pt.get<int>("max_cols_to_solve_mp");
        cplex_cores = pt.get<int>("cplex_cores");
        time_limit_in_s = pt.get<int>("time_limit_in_s");
        early_branching_timeout = pt.get<int>("early_branching_timeout");
        dummy_column_price = pt.get<long>("dummy_column_price");
        cp_acceleration_arcs_share = pt.get<double>("cp_acceleration_arcs_share");
        p_acceleration_max_arcs_share = pt.get<double>("p_acceleration_max_arcs_share");
    }
}
