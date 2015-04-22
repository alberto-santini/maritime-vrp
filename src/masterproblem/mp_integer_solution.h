//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef MP_INTEGER_SOLUTION
#define MP_INTEGER_SOLUTION

#include <vector>

class MPIntegerSolution {
public:
    double               obj_value;
    std::vector<double>   variables;
    
    MPIntegerSolution(double obj_value, const std::vector<double>& variables) : obj_value(obj_value), variables(variables) {}
};

#endif