//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef BB_NODE_H
#define BB_NODE_H

#include <memory>
#include <utility>
#include <vector>

#include <base/problem.h>
#include <branching/cycle.h>
#include <column/column_pool.h>
#include <masterproblem/mp_solver.h>
#include <subproblem/sp_solver.h>

static constexpr int no_father_lb = -999;

class BBNode {
public:
    const std::shared_ptr<const Problem> prob;
    GraphMap                    local_graphs;
    
    const std::shared_ptr<ColumnPool> pool;
    ColumnPool                  local_pool;

    VisitRuleList               unite_rules;
    VisitRuleList               separate_rules;
    IsolateRule                 isolate_rule;
    
    /*  The optimal columns selected by the LP solver with the coefficient */
    std::vector<std::pair<Column, float>> base_columns;
    /*  The optimal columns selected by the MIP solver with the coefficient */
    std::vector<std::pair<Column, float>> mip_base_columns;
    
    /*  LP Solution */
    float                       sol_value;
    /*  MIP Solution */
    float                       mip_sol_value;
    /*  LB of father node, used to determine the most promising nodes in the queue */
    float                       father_lb;
    
    /*  Used to determine if a solution is integral, or with cost < 0 */
    static constexpr float      cplex_epsilon = 0.0000001;
    
    /* Should we still try to run the ESPPRC labelling at this node? */
    bool                        try_elementary;
    
    /* Time spent on SP vs MP (avg and total) */
    std::vector<double>         all_times_spent_on_sp;
    double                      avg_time_spent_on_sp;
    double                      total_time_spent_on_sp;
    double                      total_time_spent_on_mp;
    double                      total_time_spent;
    double                      max_time_spent_by_exact_solver;
    
    int                         depth;
    
    BBNode() {}
    BBNode(const std::shared_ptr<const Problem> prob,
           const GraphMap& local_graphs,
           const std::shared_ptr<ColumnPool> pool,
           const ColumnPool& local_pool,
           const VisitRuleList& unite_rules,
           const VisitRuleList& separate_rules,
           float father_lb,
           int depth = 0,
           const IsolateRule& isolate_rule = IsolateRule(),
           bool try_elementary = true,
           double avg_time_spent_on_sp = 0,
           double total_time_spent_on_sp = 0,
           double total_time_spent_on_mp = 0,
           double total_time_spent = 0,
           double max_time_spent_by_exact_solver = 0);
    
    void solve();
    bool solve_integer(const ColumnPool& feasible_columns);
    
    bool is_feasible() const;
    bool is_integer_feasible() const;

private:
    /*  Modifies the graphs in local_prob according to the rules
        in unite_rules and separate_rules and sets the duals from
        port_duals and vc_duals */
    void make_local_graphs();
    
    /*  Removes from local_pool those columns that are not compatible
        with unite_rules and separate_rules */
    void remove_incompatible_columns();
    
    /*  Test: I want to check if there are columns with the same
        constraint coefficients and possibly different objective
        function coefficients */
    std::vector<int> column_coefficients(const Column& col);
    void check_for_duplicate_columns();
};

class BBNodeCompare {
public:
    bool operator()(const auto& n1, const auto& n2) const {
        return (n1->father_lb > n2->father_lb);
    }
};

#endif