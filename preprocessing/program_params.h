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
    int     lambda_start;
    int     lambda_end;
    int     lambda_inc;
    
    ProgramParams(string data_file_name = "data/program_params.json");
};

#endif