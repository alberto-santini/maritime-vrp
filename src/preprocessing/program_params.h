//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef PROGRAM_PARAMS_H
#define PROGRAM_PARAMS_H

#include <string>

namespace mvrp {
    class ProgramParams {
    public:
        bool parallel_labelling;
        bool early_branching;
        bool enable_cost_prize_acceleration;
        bool enable_prize_acceleration;
        int greedy_max_outarcs;
        int greedy_reruns;
        int elementary_labelling_every_n_nodes;
        int max_cols_to_solve_mp;
        int cplex_cores;
        int time_limit_in_s;
        int early_branching_timeout;
        long dummy_column_price;
        double cp_acceleration_arcs_share;
        double p_acceleration_max_arcs_share;

        ProgramParams(const std::string &data_file_name = "data/program_params.json");
    };
}

#endif
