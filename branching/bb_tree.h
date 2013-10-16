//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef BB_TREE_H
#define BB_TREE_H

#include <base/base.h>
#include <base/problem.h>
#include <branching/cycle.h>
#include <branching/bb_node.h>
#include <column/column_pool.h>

typedef priority_queue<std::shared_ptr<BBNode>, vector<std::shared_ptr<BBNode>>, BBNodeCompare> NodeQueue;
enum class BoundType { FROM_LP, FROM_MIP };

class BBTree {
public:
    std::shared_ptr<const Problem>  prob;
    std::shared_ptr<ColumnPool>     pool;
    NodeQueue                       unexplored_nodes;
    float                           ub;
    float                           lb;
    std::shared_ptr<BBNode>         node_attaining_ub;
    BoundType                       node_bound_type;
    
    BBTree();
    void explore_tree();
    
private:
    void branch_on_cycles(const Cycles& cycles, const std::shared_ptr<BBNode> current_node);
    void branch_on_fractional(const std::shared_ptr<BBNode> current_node);
    void try_to_obtain_ub(const std::shared_ptr<BBNode> current_node);
};

#endif