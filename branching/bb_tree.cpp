//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>
#include <iostream>
#include <limits>
#include <utility>

#include <branching/bb_tree.h>

BBTree::BBTree(const std::string& program_params_file_name, const std::string& data_file_name) {
    prob = std::make_shared<Problem>(program_params_file_name, data_file_name);
    ub = std::numeric_limits<double>::max();
    lb = std::numeric_limits<double>::max();

    Column dummy(prob);
    dummy.make_dummy(prob->params.dummy_column_price);
    pool = std::make_shared<ColumnPool>();
    pool->push_back(dummy);
    
    auto root_node = std::make_shared<BBNode>(prob, prob->graphs, pool, *pool, VisitRuleList(), VisitRuleList(), no_father_lb);

    unexplored_nodes.push(root_node);
    
    node_attaining_ub = root_node;
    node_bound_type = BoundType::FROM_LP;
    bb_nodes_generated = 1;
}

void BBTree::print_header() const {
	std::cout << std::setw(22) << "BB Nodes |";
	std::cout << std::setw(28) << "Lower Bound |";
	std::cout << std::setw(16) << "Upper Bound |";
	std::cout << std::setw(28) << "Gap |";
	std::cout << std::setw(10) << "|";
	std::cout << std::setw(66) << "Total time |";
	std::cout << std::setw(22) << "|";
	std::cout << std::setw(26) << "|";
    std::cout << std::setw(8) << "|" << std::endl;
	
    std::cout << std::setw(12) << "Unexplored  "; // Unexplored nodes
    std::cout << std::setw(10) << "Total |"; // Number of nodes in total
	
    std::cout << std::setw(14) << "LB at node  ";
    std::cout << std::setw(14) << "LB best |";
	
    std::cout << std::setw(16) << "UB best |";
	
    std::cout << std::setw(14) << "Gap at node  ";
    std::cout << std::setw(14) << "Gap best |";
	
    std::cout << std::setw(10) << "Columns |";
	
    std::cout << std::setw(22) << "Time spent on MP  "; // Time on MP
    std::cout << std::setw(22) << "Time spent on SP  "; // Time on SP
    std::cout << std::setw(22) << "Time spent at node |"; // Time at node

    std::cout << std::setw(22) << "Average time on SP |"; // Avg time on SP
    std::cout << std::setw(26) << "Max time on SP (exact) |"; // Max time on Exact SP
	
	std::cout << std::setw(8) << "Depth |" << std::endl;
	
	std::cout << 	"-----------*---------*" <<
					"-------------*-------------*" <<
					"---------------*" <<
					"-------------*-------------*" <<
					"---------*" <<
					"---------------------*---------------------*---------------------*" <<
					"---------------------*-------------------------*" <<
					"-------*" << std::endl;
}

void BBTree::explore_tree() {
    print_header();
    
    while(!unexplored_nodes.empty()) {
        std::cerr << "Nodes in tree: " << unexplored_nodes.size() << std::endl;
        
        auto current_node = unexplored_nodes.top();
        unexplored_nodes.pop();
        lb = current_node->father_lb;
        
        // Solve the master problem to obtain a lower bound
        current_node->solve();
        std::cerr << "\tNode LB: " << current_node->sol_value << std::endl;
        
        if(!current_node->is_feasible()) {
            // Prune by infeasibility
            std::cerr << "\t\tPruned by infeasibility" << std::endl;
            continue;
        }
        
        if(current_node->sol_value >= ub) {
            // Prune by sub-optimality
            std::cerr << "\t\tPruned by sub-optimality (UB = " << ub << ")" << std::endl;
            continue;
        }
        
        // Detect eventual cycles
        Cycles cycles;
        for(const auto& cc : current_node->base_columns) {
            if(!cc.first.dummy) {
                auto g = cc.first.sol.g;
                auto cycle = Cycle::shortest_cycle(cc.first.sol.path, g);
                if(!cycle.empty()) {
                    cycles.push_back(make_pair(cycle, g));
                }
            }
        }
        
        if(cycles.size() > 0) {
            // Solution contains cycles
            try_to_obtain_ub(current_node);
            std::cerr << "\tBranching on cycles" << std::endl;
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
                std::cerr << "\tBranching on fractional" << std::endl;
                branch_on_fractional(current_node);
            } else {
                // Solution integer
                std::cerr << "\tSolution actually integer" << std::endl;
                if(ub - current_node->sol_value > BBNode::cplex_epsilon) {
                    std::cerr << "\t\tAnd improving the UB" << std::endl;
                    ub = current_node->sol_value;
                    node_attaining_ub = current_node;
                    node_bound_type = BoundType::FROM_LP;
                }
            }
        }
        
        // Used in the first iteration when there is no father node
        if(abs(lb - no_father_lb) < std::numeric_limits<double>::epsilon()) {
            lb = current_node->sol_value;
        }
        
        auto gap_node = ((ub - current_node->sol_value) / ub) * 100;
        auto gap = ((ub - lb) / ub) * 100;
        
        print_row(*current_node, gap_node, gap);
    }
    
    std::cout << std::endl << "*** SOLUTION ***" << std::endl;
    if(node_bound_type == BoundType::FROM_LP) {
        // UB was attained as LP solution
        std::cout << "*** OBTAINED FROM LP ***" << std::endl;
        for(const auto& cc : node_attaining_ub->base_columns) {
            std::cout << cc.first << " selected with coefficient " << cc.second << std::endl;
            cc.first.sol.g->print_path(cc.first.sol.path, std::cout);
        }
    } else {
        // UB was attained as MIP solution
        std::cout << "*** OBTAINED FROM MIP ***" << std::endl;
        for(const auto& cc : node_attaining_ub->mip_base_columns) {
            std::cout << cc.first << " selected with coefficient " << cc.second << std::endl;
            cc.first.sol.g->print_path(cc.first.sol.path, std::cout);
        }
    }
}

// LIBSTD of GCC DOES NOT IMPLEMENT STD::DEFAULTFLOAT !!
inline std::ostream& defaultfloat(std::ostream& os) { os.unsetf(std::ios_base::floatfield); return os; }

void BBTree::print_row(const BBNode& current_node, double gap_node, double gap) const {
    auto print_ub = (ub < std::numeric_limits<double>::max() - 100);
    
    std::cout << std::fixed;
    std::cout << std::setw(10) << unexplored_nodes.size() << "  ";
    std::cout << std::setw(8) << bb_nodes_generated << "  ";
    std::cout << std::setw(12) << std::setprecision(2) << current_node.sol_value << "  ";
    std::cout << std::setw(12) << std::setprecision(2) << lb << "  ";
    if(print_ub) {
        std::cout << std::setw(14) << std::setprecision(2) << ub << "  ";
    } else {
        std::cout << std::setw(14) << "inf" << "  ";
    }
    std::cout << std::setw(11) << std::setprecision(4) << gap_node << "\%  ";
    std::cout << std::setw(11) << std::setprecision(4) << gap << "\%  ";
    std::cout << std::setw(8) << pool->size() << "  ";
    std::cout << std::setw(20) << std::setprecision(4) << current_node.total_time_spent_on_mp << "  ";
    std::cout << std::setw(20) << std::setprecision(4) << current_node.total_time_spent_on_sp << "  ";
    std::cout << std::setw(20) << std::setprecision(4) << current_node.total_time_spent << "  ";
    std::cout << std::setw(20) << std::setprecision(4) << current_node.avg_time_spent_on_sp << "  ";
    std::cout << std::setw(24) << std::setprecision(4) << current_node.max_time_spent_by_exact_solver << "  ";
    std::cout << std::setw(6) << current_node.depth << "  " << std::endl;
    defaultfloat(std::cout);
}

void BBTree::branch_on_cycles(const Cycles& cycles, std::shared_ptr<BBNode> current_node) {    
    auto shortest = std::min_element(cycles.begin(), cycles.end(),
        [] (const auto& c1, const auto& c2) {
            return(c1.first.size() < c2.first.size());
        });

    const auto g = shortest->second;    
    VisitRuleList unite_rules, separate_rules;

    std::cerr << "\t\tShortest cycle of length " << shortest->first.size() << " on graph for vc " << g->vessel_class->name << ": ";
    Cycle::print_cycle(shortest->first, g, std::cerr);

    for(auto fix_forb = shortest->first.begin(); fix_forb != shortest->first.end(); ++fix_forb) {
        std::cerr << "\t\t\tCreating child node:" << std::endl;
        
        for(auto fix_impo = shortest->first.begin(); fix_impo != fix_forb; ++fix_impo) {
            auto n_source_impo = g->graph[source(*fix_impo, g->graph)];
            auto n_target_impo = g->graph[target(*fix_impo, g->graph)];
            std::cerr << "\t\t\t\tForcing the traversal of " << n_source_impo->port->name << " -> " << n_target_impo->port->name << std::endl;
            unite_rules.push_back(std::make_pair(n_source_impo, n_target_impo));
        }
        
        auto n_source_forb = g->graph[source(*fix_forb, g->graph)];
        auto n_target_forb = g->graph[target(*fix_forb, g->graph)];
        std::cerr << "\t\t\t\tForbidding the traversal of " << n_source_forb->port->name << " -> " << n_target_forb->port->name << std::endl;
        separate_rules.push_back(std::make_pair(n_source_forb, n_target_forb));
        
        unexplored_nodes.push(
            std::make_shared<BBNode>(
                current_node->prob,
                current_node->local_graphs,
                current_node->pool,
                current_node->local_pool,
                unite_rules,
                separate_rules,
                current_node->sol_value,
                current_node->depth + 1
            )
        );
        bb_nodes_generated++;
    }
}

void BBTree::branch_on_fractional(const std::shared_ptr<BBNode> current_node) {
    // Detect the most fractional column
    auto cc_it = std::max_element(current_node->base_columns.begin(), current_node->base_columns.end(),
        [] (const auto& cc1, const auto& cc2) {
            return (fabs(cc1.second - 0.5) > fabs(cc2.second - 0.5));
        });
    
    for(const auto& e : cc_it->first.sol.path) {
        const auto g = cc_it->first.sol.g;
        auto n = g->graph[target(e, g->graph)];
        if(n->n_type != NodeType::REGULAR_PORT) {
            continue;
        }
        for(const auto& cc : current_node->base_columns) {
            for(const auto& e_inner : cc.first.sol.path) {
                const auto g_inner = cc.first.sol.g;
                auto n_inner = g_inner->graph[target(e_inner, g_inner->graph)];
                if(n_inner->same_row_as(*n)) {
                    auto n_src = g->graph[source(e, g->graph)];
                    auto n_inner_src = g_inner->graph[source(e_inner, g_inner->graph)];
                    if(!n_inner_src->same_row_as(*n_src)) {
                        std::cerr << "\t\tPort " << n->port->name << " visited by 2 routes from 2 different ports - acting on graph for vc " << g->vessel_class->name << ":" << std::endl;
                        
                        VisitRuleList v_rules;
                        IsolateRule i_rule;
                        v_rules.push_back(make_pair(n_src, n));
                        i_rule = make_pair(n_src, n);
                        
                        std::cerr << "\t\t\tCreating child node 1:" << std::endl;
                        unexplored_nodes.push(
                            std::make_shared<BBNode>(
                                current_node->prob,
                                current_node->local_graphs,
                                current_node->pool,
                                current_node->local_pool,
                                VisitRuleList(),
                                v_rules,
                                current_node->sol_value,
                                current_node->depth + 1
                            )
                        );  
                        
                        std::cerr << "\t\t\tCreating child node 2:" << std::endl;
                        unexplored_nodes.push(
                            std::make_shared<BBNode>(
                                current_node->prob,
                                current_node->local_graphs,
                                current_node->pool,
                                current_node->local_pool,
                                v_rules,
                                VisitRuleList(),
                                current_node->sol_value,
                                current_node->depth + 1,
                                i_rule
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
    
    for(const auto& c : current_node->local_pool) {
        if(!c.dummy && !c.has_cycles()) {
            feasible_columns.push_back(Column(c.prob, c.sol, "MIP elimination", ColumnOrigin::MIP));
        }
    }
    
    if(feasible_columns.size() <= prob->params.max_cols_to_solve_mp) {
        if(current_node->solve_integer(feasible_columns)) {
            std::cerr << "\tNode UB: " << current_node->mip_sol_value << std::endl;
            if(ub - current_node->mip_sol_value > BBNode::cplex_epsilon) {
                std::cerr << "\t\tImproving the UB" << std::endl;
                ub = current_node->mip_sol_value;
                node_attaining_ub = current_node;
                node_bound_type = BoundType::FROM_MIP;
                for(const auto& cc : node_attaining_ub->mip_base_columns) {
                    std::cerr << "\t\t\t " << cc.first << " with coeff " << cc.second << std::endl;
                }
            }
        } else {
            std::cerr << "\tMIP infeasible" << std::endl;
        }
    } else {
        std::cerr << "\tToo many columns to solve MIP" << std::endl;
    }
}
