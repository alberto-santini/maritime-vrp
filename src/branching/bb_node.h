//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef BB_NODE_H
#define BB_NODE_H

#include <memory>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

#include "../base/problem.h"
#include "../column/column_pool.h"
#include "../masterproblem/mp_solver.h"
#include "../subproblem/sp_solver.h"
#include "../branching/branching_rule.h"

namespace mvrp {
    class BBNode {
        static constexpr double EPS = 1e-9;

    public:
        std::shared_ptr<const Problem> prob;
        ErasedEdgesMap local_erased_edges;
        std::vector<PortWithType> ports_with_equality;

        std::shared_ptr<ColumnPool> pool;
        ColumnPool local_pool;

        std::shared_ptr<BranchingRule> branching_rule;

        /*  The optimal columns selected by the LP solver with the coefficient */
        std::vector<std::pair<Column, double>> base_columns;
        /*  The optimal columns selected by the MIP solver with the coefficient */
        std::vector<std::pair<Column, double>> mip_base_columns;

        /*  LP Solution */
        double sol_value;
        /*  MIP Solution */
        double mip_sol_value;
        /*  LB of father node, used to determine the most promising nodes in the queue. The root node does not have any. */
        boost::optional<double> father_lb;

        /* Depth in the BB tree */
        int depth;

        /* Node name */
        std::string name;

        /*  Used to determine if a solution is integral, or with cost < 0 */
        static constexpr double cplex_epsilon = 0.000001;

        /* Should we still try to run the ESPPRC labelling at this node? */
        bool try_elementary;

        /* Time spent on SP vs MP (avg and total) */
        std::vector<double> all_times_spent_on_sp;
        double avg_time_spent_on_sp;
        double total_time_spent_on_sp;
        double total_time_spent_on_mp;
        double total_time_spent;
        double max_time_spent_by_exact_solver;

        BBNode() {}

        BBNode(std::shared_ptr<const Problem> prob,
               const ErasedEdgesMap &local_erased_edges,
               std::vector<PortWithType> ports_with_equality,
               std::shared_ptr<ColumnPool> pool,
               const ColumnPool &local_pool,
               std::shared_ptr<BranchingRule> branching_rule = nullptr,
               boost::optional<double> father_lb = boost::none,
               int depth = 0,
               std::string name = "root",
               bool try_elementary = true,
               double avg_time_spent_on_sp = 0,
               double total_time_spent_on_sp = 0,
               double total_time_spent_on_mp = 0,
               double total_time_spent = 0,
               double max_time_spent_by_exact_solver = 0);

        BBNode(const BBNode& father, std::shared_ptr<BranchingRule> branching_rule, std::string name);

        void solve(unsigned int node_number);

        bool solve_integer(const ColumnPool &feasible_columns);

        bool is_feasible() const;

        bool is_integer_feasible() const;

        bool has_fractional_solution() const;

    private:
        /*  Modifies the erased edges according to the branching rules */
        void make_local_erased_edges();

        /*  Removes from local_pool those columns that are not compatible
            with the branching rules */
        void remove_incompatible_columns();

        /* Determines which ports need an = constraint */
        void determine_equality_constraints();

        /*  Test: I want to check if there are columns with the same
            constraint coefficients and possibly different objective
            function coefficients */
        std::vector<int> column_coefficients(const Column &col);

        void remove_duplicate_columns();
    };

    class BBNodeCompare {
    public:
        bool operator()(const std::shared_ptr<BBNode> &n1, const std::shared_ptr<BBNode> &n2) const {
            if(!n1->father_lb) { return true; }
            if(!n2->father_lb) { return false; }
            return (*n1->father_lb > *n2->father_lb);
        }
    };
}
#endif
