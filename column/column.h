//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef COLUMN_H
#define COLUMN_H

#include <memory>
#include <string>
#include <vector>

#include <base/problem.h>
#include <column/solution.h>

enum class ColumnOrigin { FAST_H, ESPPRC, SPPRC_SMART, SPPRC_RED, SPPRC, MIP, NONE };

class Column {
public:
    std::shared_ptr<const Problem>  prob;
    Solution                        sol;
    float                           obj_coeff;
    std::vector<float>              port_coeff;
    std::vector<float>              vc_coeff;
    bool                            dummy;
    std::string                     created_by;
    ColumnOrigin                    origin; 
    
    Column() : origin(ColumnOrigin::NONE) {}
    Column(std::shared_ptr<const Problem> prob) : prob(prob) {}
    Column(std::shared_ptr<const Problem> prob, const Solution& sol, const std::string& created_by, ColumnOrigin origin = ColumnOrigin::NONE);
    Column(std::shared_ptr<const Problem> prob,
           const Solution& sol,
           float obj_coeff,
           const std::vector<float>& port_coeff,
           const std::vector<float>& vc_coeff,
           bool dummy,
           const std::string& created_by,
           ColumnOrigin origin) : prob(prob), sol(sol), obj_coeff(obj_coeff), port_coeff(port_coeff), vc_coeff(vc_coeff), dummy(dummy), created_by(created_by), origin(origin) {}
    
    void make_dummy(float huge_cost);
    
    bool is_compatible_with_unite_rule(const VisitRule& vr) const;
    bool is_compatible_with_separate_rule(const VisitRule& vr) const;
    
    bool has_cycles() const;
};

std::ostream& operator<<(std::ostream& out, const Column& c);

#endif