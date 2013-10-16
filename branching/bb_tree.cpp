//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <branching/bb_tree.h>

BBTree::BBTree() {
    prob = make_shared<Problem>();
    ub = numeric_limits<float>::max();
    lb = numeric_limits<float>::max();

    Column dummy(prob);
    dummy.make_dummy(prob->params.dummy_column_price);
    pool = make_shared<ColumnPool>();
    pool->push_back(dummy);

    std::shared_ptr<BBNode> root_node = make_shared<BBNode>(prob, prob->graphs, pool, *pool, VisitRuleList(), VisitRuleList(), numeric_limits<float>::max());

    unexplored_nodes.push(root_node);
    
    node_attaining_ub = root_node;
    node_bound_type = BoundType::FROM_LP;
    bb_nodes_generated = 1;
}

void BBTree::explore_tree() {
    cout << setw(20) << "# unexplored nodes";
    cout << setw(14) << "LB at node";
    cout << setw(14) << "UB";
    cout << setw(20) << "Gap at node";
    cout << setw(20) << "Gap";
    cout << setw(20) << "# columns in pool";
    cout << setw(14) << "# BB Nodes" << endl;
    
    while(!unexplored_nodes.empty()) {
        cerr << "Nodes in tree: " << unexplored_nodes.size() << endl;
        
        std::shared_ptr<BBNode> current_node = unexplored_nodes.top();
        unexplored_nodes.pop();
        lb = current_node->sol_value;
        
        // Solve the master problem to obtain a lower bound
        current_node->solve();
        cerr << "\tNode LB: " << current_node->sol_value << endl;
        
        if(!current_node->is_feasible()) {
            // Prune by infeasibility
            cerr << "\t\tPruned by infeasibility" << endl;
            continue;
        }
        
        if(current_node->sol_value >= ub) {
            // Prune by sub-optimality
            cerr << "\t\tPruned by sub-optimality (UB = " << ub << ")" << endl;
            continue;
        }
        
        // Detect eventual cycles
        Cycles cycles;
        for(const auto& cc : current_node->base_columns) {
            if(!cc.first.dummy) {
                std::shared_ptr<const Graph> g = cc.first.sol.g;
                Path cycle = Cycle::shortest_cycle(cc.first.sol.path, g);
                if(!cycle.empty()) {
                    cycles.push_back(make_pair(cycle, g));
                }
            }
        }
        
        if(cycles.size() > 0) {
            // Solution contains cycles
            try_to_obtain_ub(current_node);
            cerr << "\tBranching on cycles" << endl;
            branch_on_cycles(cycles, current_node);
        } else {
            // Check if the solution is feasible (i.e. integer vs fractional)
            bool fractional = false;
            for(const auto& cc : current_node->base_columns) {
                if(fabs(cc.second - 0) > BBNode::cplex_epsilon && fabs(cc.second - 1) > BBNode::cplex_epsilon) {
                    fractional = true;
                    break;
                }
            }
            
            if(fractional) {
                // Solution is fractional
                try_to_obtain_ub(current_node);
                cerr << "\tBranching on fractional" << endl;
                branch_on_fractional(current_node);
            } else {
                // Solution integer
                cerr << "\tSolution actually integer" << endl;
                if(ub - current_node->sol_value > BBNode::cplex_epsilon) {
                    cerr << "\t\tAnd improving the UB" << endl;
                    ub = current_node->sol_value;
                    node_attaining_ub = current_node;
                    node_bound_type = BoundType::FROM_LP;
                }
            }
        }
        
        float gap_node = ((ub - current_node->sol_value) / current_node->sol_value) * 100;
        float gap = ((ub - lb) / lb) * 100;
        
        cout << setw(20) << unexplored_nodes.size();
        cout << setw(20) << current_node->sol_value;
        cout << setw(20) << ub;
        cout << setw(19) << setprecision(6) << gap_node << "\%";
        cout << setw(19) << setprecision(6) << gap << "\%";
        cout << setw(20) << pool->size();
        cout << setw(14) << bb_nodes_generated << endl;
    }
    
    cout << endl << "*** SOLUTION ***" << endl;
    if(node_bound_type == BoundType::FROM_LP) {
        // UB was attained as LP solution
        cout << "*** OBTAINED FROM LP ***" << endl;
        for(const auto& cc : node_attaining_ub->base_columns) {
            cout << cc.first << " selected with coefficient " << cc.second << endl;
            cc.first.sol.g->print_path(cc.first.sol.path, cout);
        }
    } else {
        // UB was attained as MIP solution
        cout << "*** OBTAINED FROM MIP ***" << endl;
        for(const auto& cc : node_attaining_ub->mip_base_columns) {
            cout << cc.first << " selected with coefficient " << cc.second << endl;
            cc.first.sol.g->print_path(cc.first.sol.path, cout);
        }
    }
}

void BBTree::branch_on_cycles(const Cycles& cycles, const std::shared_ptr<BBNode> current_node) {    
    Cycles::const_iterator shortest = min_element(cycles.begin(), cycles.end(),
        [] (const pair<Path, const std::shared_ptr<const Graph>>& c1, const pair<Path, const std::shared_ptr<const Graph>>& c2) {
            return(c1.first.size() < c2.first.size());
        });

    const std::shared_ptr<const Graph> g = shortest->second;    
    Path::const_iterator fix_forb, fix_impo;
    VisitRuleList unite_rules, separate_rules;

    cerr << "\t\tShortest cycle of length " << shortest->first.size() << " on graph for vc " << g->vessel_class->name << ": ";
    Cycle::print_cycle(shortest->first, g, cerr);

    for(fix_forb = shortest->first.begin(); fix_forb != shortest->first.end(); ++fix_forb) {
        cerr << "\t\t\tCreating child node:" << endl;
        
        for(fix_impo = shortest->first.begin(); fix_impo != fix_forb; ++fix_impo) {
            std::shared_ptr<Node> n_source_impo = g->graph[source(*fix_impo, g->graph)];
            std::shared_ptr<Node> n_target_impo = g->graph[target(*fix_impo, g->graph)];
            cerr << "\t\t\t\tForcing the traversal of " << n_source_impo->port->name << " -> " << n_target_impo->port->name << endl;
            unite_rules.push_back(make_pair(n_source_impo, n_target_impo));
        }
        
        std::shared_ptr<Node> n_source_forb = g->graph[source(*fix_forb, g->graph)];
        std::shared_ptr<Node> n_target_forb = g->graph[target(*fix_forb, g->graph)];
        cerr << "\t\t\t\tForbidding the traversal of " << n_source_forb->port->name << " -> " << n_target_forb->port->name << endl;
        separate_rules.push_back(make_pair(n_source_forb, n_target_forb));
        
        unexplored_nodes.push(
            make_shared<BBNode>(
                current_node->prob,
                current_node->local_graphs,
                current_node->pool,
                current_node->local_pool,
                unite_rules,
                separate_rules,
                current_node->sol_value
            )
        );
        bb_nodes_generated++;
    }
}

void BBTree::branch_on_fractional(const std::shared_ptr<BBNode> current_node) {
    // Detect the most fractional column
    auto cc_it = max_element(current_node->base_columns.begin(), current_node->base_columns.end(),
        [] (const pair<Column, float>& cc1, const pair<Column, float>& cc2) {
            return (fabs(cc1.second - 0.5) > fabs(cc2.second - 0.5));
        });
    
    for(const Edge& e : cc_it->first.sol.path) {
        const std::shared_ptr<const Graph> g = cc_it->first.sol.g;
        std::shared_ptr<Node> n = g->graph[target(e, g->graph)];
        if(n->n_type != NodeType::REGULAR_PORT) {
            continue;
        }
        for(const auto& cc : current_node->base_columns) {
            for(const Edge& e_inner : cc.first.sol.path) {
                const std::shared_ptr<const Graph> g_inner = cc.first.sol.g;
                std::shared_ptr<Node> n_inner = g_inner->graph[target(e_inner, g_inner->graph)];
                if(n_inner->same_row_as(*n)) {
                    std::shared_ptr<Node> n_src = g->graph[source(e, g->graph)];
                    std::shared_ptr<Node> n_inner_src = g_inner->graph[source(e_inner, g_inner->graph)];
                    if(!n_inner_src->same_row_as(*n_src)) {
                        cerr << "\t\tPort " << n->port->name << " visited by 2 routes from 2 different ports - acting on graph for vc " << g->vessel_class->name << ":" << endl;
                        VisitRuleList unite_rules_u, separate_rules_u, unite_rules_s, separate_rules_s;
                        unite_rules_u.push_back(make_pair(n_src, n));
                        separate_rules_s.push_back(make_pair(n_src, n));
                        
                        cerr << "\t\t\tForcing the traversal of " << n_src->port->name << " -> " << n->port->name << endl;
                        // Forcing the traversal
                        unexplored_nodes.push(
                            make_shared<BBNode>(
                                current_node->prob,
                                current_node->local_graphs,
                                current_node->pool,
                                current_node->local_pool,
                                unite_rules_u,
                                separate_rules_u,
                                current_node->sol_value
                            )
                        );
                        
                        cerr << "\t\t\tForbidding the traversal of " << n_src->port->name << " -> " << n->port->name << endl;
                        // Forbidding the traversal        
                        unexplored_nodes.push(
                            make_shared<BBNode>(
                                current_node->prob,
                                current_node->local_graphs,
                                current_node->pool,
                                current_node->local_pool,
                                unite_rules_s,
                                separate_rules_s,
                                current_node->sol_value
                            )
                        );
                                
                        bb_nodes_generated += 2;
                                
                        goto exit_nested_loops;
                    }
                }
            }
        }
    }
    
    exit_nested_loops:
    {}
}

void BBTree::try_to_obtain_ub(const std::shared_ptr<BBNode> current_node) {
    ColumnPool feasible_columns;
    
    for(const Column& c : current_node->local_pool) {
        if(!c.dummy && !c.has_cycles()) {
            feasible_columns.push_back(Column(c.prob, c.sol, "MIP elimination"));
        }
    }
    
    if(feasible_columns.size() <= prob->params.max_cols_to_solve_mp) {
        if(current_node->solve_integer(feasible_columns)) {
            cerr << "\tNode UB: " << current_node->mip_sol_value << endl;
            if(ub - current_node->mip_sol_value > BBNode::cplex_epsilon) {
                cerr << "\t\tImproving the UB" << endl;
                ub = current_node->mip_sol_value;
                node_attaining_ub = current_node;
                node_bound_type = BoundType::FROM_MIP;
                for(const auto&cc : node_attaining_ub->mip_base_columns) {
                    cerr << "\t\t\t " << cc.first << " with coeff " << cc.second << endl;
                }
            }
        } else {
            cerr << "\tMIP infeasible" << endl;
        }
    } else {
        cerr << "\tToo many columns to solve MIP" << endl;
    }
}
