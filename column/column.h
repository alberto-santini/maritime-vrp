//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef COLUMN_H
#define COLUMN_H

#include <base/base.h>
#include <base/problem.h>
#include <column/solution.h>

class Column {
public:
    const Problem&  prob;
    Solution        sol;
    float           obj_coeff;
    vector<float>   port_coeff;
    vector<float>   vc_coeff;
    bool            dummy;
    
    Column(const Problem& prob) : prob(prob) {}
    Column(const Problem& prob, const Solution sol);
    Column(const Problem& prob,
           Solution sol,
           float obj_coeff,
           vector<float> port_coeff,
           vector<float> vc_coeff,
           bool dummy) : prob(prob), sol(sol), obj_coeff(obj_coeff), port_coeff(port_coeff), vc_coeff(vc_coeff), dummy(dummy) {}
    ~Column() {}
    void operator=(const Column& other);
    
    void make_dummy(const float huge_cost);
    
    bool is_compatible_with_unite_rule(VisitRule vr) const;
    bool is_compatible_with_separate_rule(VisitRule vr) const;
    
    Column transfer_to(const Problem& other_prob) const;
};

ostream& operator<<(ostream& out, const Column& c);

#endif