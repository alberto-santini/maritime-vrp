//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <branching/bb_node.h>

BBNode::BBNode(const Problem& prob, const Problem local_prob, ColumnPool& pool, const ColumnPool local_pool, const VisitRule unite_rule, const VisitRule separate_rule) : prob(prob), local_prob(local_prob), pool(pool), local_pool(local_pool), unite_rule(unite_rule), separate_rule(separate_rule) {
    make_local_graphs();
    remove_incompatible_columns();
}

void BBNode::make_local_graphs() {
    if(unite_rule.first != nullptr) {
        // Unite rule applies
        Graph& g = local_prob.graphs.at(unite_rule.first->vessel_class);
        g.unite_ports(unite_rule, g);
    }

    if(separate_rule.first != nullptr) {
        // Separate rule applies
        Graph& g = local_prob.graphs.at(separate_rule.first->vessel_class);
        g.separate_ports(separate_rule, g);
    }
}

void BBNode::remove_incompatible_columns() {
    ColumnPool new_local_pool;
    
    for(const Column& c : local_pool) {
        if(c.is_compatible_with_unite_rule(unite_rule) && c.is_compatible_with_separate_rule(separate_rule)) {
            new_local_pool.push_back(c.transfer_to(local_prob));
        }
    }
    
    local_pool = new_local_pool;
}

void BBNode::solve() {
    // Clear any eventual previous solutions
    base_columns = vector<pair<Column, float>>();
    sol_value = 0;
    
    MPSolver mp_solv(local_prob);
    MPLinearSolution sol = mp_solv.solve_lp(local_pool);

    bool node_explored = false;
    while(!node_explored) {
        for(auto& vg : local_prob.graphs) {
            vg.second.graph[graph_bundle].port_duals = sol.port_duals;
            vg.second.graph[graph_bundle].vc_dual = sol.vc_duals.at(vg.first);
        }

        SPSolver sp_solv(local_prob);
        bool sp_found_columns;

        sp_found_columns = sp_solv.solve(local_pool);

        if(sp_found_columns) {
            // If new columns are found, solve the LP again
            sol = mp_solv.solve_lp(local_pool);
        } else {
            // Otherwise, the exploration is done
            sol_value = sol.obj_value;

            for(int i = 0; i < sol.variables.size(); i++) {
                if(sol.variables[i] > BBNode::cplex_epsilon) {
                    base_columns.push_back(make_pair(local_pool[i].transfer_to(prob), sol.variables[i]));
                }
            }

            node_explored = true;
        }
    }
}

void BBNode::solve_integer() {
    // Clear any eventual previous solutions
    base_columns = vector<pair<Column, float>>();
    sol_value = 0;
    
    MPSolver mp_solv(local_prob);
    MPIntegerSolution sol = mp_solv.solve_mip(local_pool);

    sol_value = sol.obj_value;

    for(int i = 0; i < sol.variables.size(); i++) {
        if(sol.variables[i] > BBNode::cplex_epsilon) {
            base_columns.push_back(make_pair(local_pool[i].transfer_to(prob), sol.variables[i]));
        }
    }
}