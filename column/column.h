//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef COLUMN_H
#define COLUMN_H

class Column {
public:
    Problem&        prob;
    Solution        sol;
    float           obj_coeff;
    vector<float>   port_coeff;
    vector<float>   vc_coeff;
    bool            dummy;
    
    Column(const Problem& prob) {}
    Column(const Problem& prob, const Solution sol);
    
    void make_dummy(const float huge_cost);
};

#endif