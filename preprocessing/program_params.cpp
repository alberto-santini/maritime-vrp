//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <preprocessing/program_params.h>

ProgramParams::ProgramParams(string data_file_name) {
    ptree pt;
    read_json(data_file_name, pt);
    
    delta = pt.get<int>("delta");
    theta = pt.get<int>("theta");
    lambda_start = pt.get<float>("lambda_start");
    lambda_end = pt.get<float>("lambda_end");
    lambda_inc = pt.get<float>("lambda_increment");
    max_cols_to_solve_mp = pt.get<int>("max_cols_to_solve_mp");
    gap_to_be_considered_optimal = pt.get<float>("gap_to_be_considered_optimal");
    dummy_column_price = pt.get<int>("dummy_column_price");
}