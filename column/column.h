//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef COLUMN_H
#define COLUMN_H

#include <base/base.h>
#include <base/problem.h>
#include <column/solution.h>

enum class ColumnOrigin { FAST_H, ESPPRC, SPPRC_SMART, SPPRC_RED, SPPRC, MIP, NONE };

class Column {
public:
    std::shared_ptr<const Problem> prob;
    Solution        sol;
    float           obj_coeff;
    vector<float>   port_coeff;
    vector<float>   vc_coeff;
    bool            dummy;
    string          created_by;
    ColumnOrigin    origin;
    
    Column() : origin(ColumnOrigin::NONE) {}
    Column(const std::shared_ptr<const Problem> prob) : prob(prob) {}
    Column(const std::shared_ptr<const Problem> prob, const Solution sol, const string created_by, const ColumnOrigin origin = ColumnOrigin::NONE);
    Column(const std::shared_ptr<const Problem> prob,
           const Solution sol,
           const float obj_coeff,
           const vector<float> port_coeff,
           const vector<float> vc_coeff,
           const bool dummy,
           const string created_by,
           const ColumnOrigin origin) : prob(prob), sol(sol), obj_coeff(obj_coeff), port_coeff(port_coeff), vc_coeff(vc_coeff), dummy(dummy), created_by(created_by), origin(origin) {}
    ~Column() {}
    
    void make_dummy(const float huge_cost);
    
    bool is_compatible_with_unite_rule(VisitRule vr) const;
    bool is_compatible_with_separate_rule(VisitRule vr) const;
    
    bool has_cycles() const;
};

ostream& operator<<(ostream& out, const Column& c);

#endif