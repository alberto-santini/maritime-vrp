//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef PROGRAM_PARAMS_H
#define PROGRAM_PARAMS_H

#include <string>

class ProgramParams {
public:
    int     delta;
    int     theta;
    int     max_cols_to_solve_mp;
    int     dummy_column_price;
    int     cplex_cores;
    bool    try_elementary_labelling;
    bool    try_smart_graph_reduction;
    bool    try_reduced_labelling;
    bool    use_stop_arcs;
    double  smart_min_chance;
    double  smart_max_chance;
    
    ProgramParams(const std::string& data_file_name = "data/program_params.json");
};

#endif