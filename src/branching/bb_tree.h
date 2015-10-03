//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef BB_TREE_H
#define BB_TREE_H

#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <base/problem.h>
#include <branching/cycle.h>
#include <branching/bb_node.h>
#include <column/column_pool.h>

typedef std::priority_queue<std::shared_ptr<BBNode>, std::vector<std::shared_ptr<BBNode>>, BBNodeCompare> NodeQueue;
enum class BoundType { FROM_LP, FROM_MIP };

class BBTree {
public:
    std::shared_ptr<const Problem>  prob;
    std::shared_ptr<ColumnPool>     pool;
    NodeQueue                       unexplored_nodes;
    double                          ub;
    double                          lb;
    int                             bb_nodes_generated;
    std::shared_ptr<BBNode>         node_attaining_ub;
    BoundType                       node_bound_type;
    std::string                     instance_file_name;
    double                          gap_at_root;
    double                          gap;
    double                          elapsed_time;
    int                             max_depth;
    double                          total_time_on_master;
    double                          total_time_on_pricing;
    
    BBTree(const std::string& program_params_file_name, const std::string& data_file_name);
    void explore_tree();
    
private:
    void branch_on_cycles(const Cycles& cycles, std::shared_ptr<BBNode> current_node);
    void branch_on_fractional(std::shared_ptr<BBNode> current_node);
    void try_to_obtain_ub(std::shared_ptr<BBNode> current_node);
    void print_header() const;
    void print_row(const BBNode& current_node, double gap_node) const;
    void print_summary() const;
    void print_results() const;
};

#endif