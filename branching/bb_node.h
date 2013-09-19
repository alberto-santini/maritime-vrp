//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef BB_NODE_H
#define BB_NODE_H

#include <base/base.h>
#include <base/problem.h>
#include <column/column.h>

class BBNode {
public:
    Problem         prob;   // Note: we *copy* the original graphs and then we modify them
    ColumnPool&     pool;
    ColumnPool      local_pool;
    VisitRuleList   unite_rules;
    VisitRuleList   separate_rules;
    
    BBNode(const Problem prob, ColumnPool& pool, const VisitRuleList unite_rules, const VisitRuleList separate_rules);
    
    void populate_pool();
    
private:
    void make_local_graphs();
    void copy_compatible_columns();
    void generate_nrc_columns();
};

#endif