//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef PROGRAM_PARAMS_H
#define PROGRAM_PARAMS_H

#include <base/base.h>

class ProgramParams {
public:
    int     delta;
    int     theta;
    float   lambda_start;
    float   lambda_end;
    float   lambda_inc;
    int     max_cols_to_solve_mp;
    float   gap_to_be_considered_optimal; // This is given in %
    int     dummy_column_price;
    
    ProgramParams(string data_file_name = "data/program_params.json");
};

#endif