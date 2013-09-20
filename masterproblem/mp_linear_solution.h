//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef MP_LINEAR_SOLUTION
#define MP_LINEAR_SOLUTION

#include <base/base.h>

class MPLinearSolution {
public:
    float           obj_value;
    PortDuals       port_duals;
    VcDuals         vc_duals;
    vector<float>   variables;
    
    MPLinearSolution(const float obj_value, const PortDuals port_duals, const VcDuals vc_duals, const vector<float> variables) : obj_value(obj_value), port_duals(port_duals), vc_duals(vc_duals), variables(variables) {}
};

#endif