//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef COLUMN_H
#define COLUMN_H

#include <memory>
#include <string>
#include <vector>

#include "../base/problem.h"
#include "solution.h"

namespace mvrp {
    enum class ColumnOrigin {
        FAST_H, ESPPRC, SPPRC_SMART, SPPRC_RED, SPPRC, MIP, NONE
    };

    struct Column {
        std::shared_ptr<const Problem> prob;
        Solution sol;
        double obj_coeff;
        std::vector<double> port_coeff;
        std::vector<double> vc_coeff;
        bool dummy;
        ColumnOrigin origin;

        Column() : origin(ColumnOrigin::NONE) {}

        Column(std::shared_ptr<const Problem> prob) : prob(prob) {}

        Column(std::shared_ptr<const Problem> prob, const Solution &sol, ColumnOrigin origin = ColumnOrigin::NONE);

        Column(std::shared_ptr<const Problem> prob, const Solution &sol, double obj_coeff, const std::vector<double> &port_coeff, const std::vector<double> &vc_coeff, bool dummy, ColumnOrigin origin) :
               prob(prob), sol(sol), obj_coeff(obj_coeff), port_coeff(port_coeff), vc_coeff(vc_coeff), dummy(dummy), origin(origin) {}

        void make_dummy(double huge_cost);
        bool has_cycles() const;
    };

    std::ostream &operator<<(std::ostream &out, const Column &c);
}

#endif