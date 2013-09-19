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
}