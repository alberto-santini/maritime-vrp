//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef MP_INTEGER_SOLUTION
#define MP_INTEGER_SOLUTION

#include <vector>

class MPIntegerSolution {
public:
    float                obj_value;
    std::vector<float>   variables;
    
    MPIntegerSolution(float obj_value, const std::vector<float>& variables) : obj_value(obj_value), variables(variables) {}
};

#endif