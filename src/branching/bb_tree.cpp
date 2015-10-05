//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <utility>
#include <chrono>
#include <ctime>

#include <branching/bb_tree.h>

BBTree::BBTree(const std::string& program_params_file_name, const std::string& data_file_name) : instance_file_name{data_file_name} {
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
    elapsed_time = 0;
    max_depth = 0;
    total_time_on_master = 0;
    total_time_on_pricing = 0;
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

void BBTree::update_lb(std::shared_ptr<BBNode> current_node, unsigned int node_number) {
    // Root node. Global LB is node's LB.
    if(node_number <= 1u) {
        lb = current_node->sol_value;
        return;
    }
    
    // Ran out of nodes => Tree explored completely => Solution found! => LB = UB
    if(unexplored_nodes.empty()) {
        lb = ub;
        return;
    }
    
    lb = unexplored_nodes.top()->father_lb;
}

void BBTree::explore_tree() {
    using namespace std::chrono;
    
    print_header();
    
    auto node_number = 0u;
    auto start_time = high_resolution_clock::now();
    
    while(!unexplored_nodes.empty()) {
        std::cerr << "Nodes in tree: " << unexplored_nodes.size() << std::endl;
        
        auto current_node = unexplored_nodes.top();
        unexplored_nodes.pop();
        
        // Solve the master problem to obtain a lower bound
        current_node->solve(node_number++);
        std::cerr << "\tNode LB: " << current_node->sol_value << std::endl;
        
        if(current_node->depth > max_depth) {
            max_depth = current_node->depth;
        }
 
        if(!current_node->is_feasible()) {
            // Prune by infeasibility
            std::cerr << "\t\tPruned by infeasibility (optimal LP solution contains dummy column)" << std::endl;
            
            if(node_number == 1u) {
                // Infeasible at root node!
                std::cout << "Root node infeasible" << std::endl;
            }
            
            update_lb(current_node, node_number);
            gap = std::abs((ub - lb) / ub) * 100;
            
            continue;
        }
        
        if(current_node->sol_value >= ub) {
            // Prune by sub-optimality
            std::cerr << "\t\tPruned by sub-optimality (UB = " << ub << ")" << std::endl;
            
            update_lb(current_node, node_number);
            gap = std::abs((ub - lb) / ub) * 100;
            
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
        
        update_lb(current_node, node_number);
                
        auto gap_node = std::abs((ub - current_node->sol_value) / ub) * 100;
        gap = std::abs((ub - lb) / ub) * 100;
                
        if(node_number == 1u) {
            gap_at_root = gap_node;
        }
        
        total_time_on_master += current_node->total_time_spent_on_mp;
        total_time_on_pricing += current_node->total_time_spent_on_sp;
        
        print_row(*current_node, gap_node);
        
        auto curr_time = high_resolution_clock::now();
        auto el_time = duration_cast<duration<double>>(curr_time - start_time).count();
        
        if(el_time > prob->params.time_limit_in_s) {
            std::cerr << std::endl << "Over time limit! " << el_time << std::endl;
            break;
        }
    }
    
    auto end_time = high_resolution_clock::now();
    elapsed_time = duration_cast<duration<double>>(end_time - start_time).count();
    
    print_summary();
    print_results();
}

// LIBSTD of GCC DOES NOT IMPLEMENT STD::DEFAULTFLOAT !!
inline std::ostream& defaultfloat(std::ostream& os) { os.unsetf(std::ios_base::floatfield); return os; }

void BBTree::print_results() const {
    std::ofstream results_file;
    results_file.open("results.txt", std::ios::out | std::ios::app);
    
    auto elements = std::vector<std::string>();
    auto ss = std::stringstream(instance_file_name);
    auto item = std::string();

    while(std::getline(ss, item, '/')) {
        elements.push_back(item);
    }
    
    ss = std::stringstream(std::string(elements.back()));
    elements = std::vector<std::string>();
    item = std::string();
        
    while(std::getline(ss, item, '.')) {
        elements.push_back(item);
    }
    
    ss = std::stringstream(std::string(elements[0]));
    elements = std::vector<std::string>();
    item = std::string();
    
    while(std::getline(ss, item, '_')) {
        elements.push_back(item);
    }
    
    assert(elements.size() == 10u);
    
    // Scenario name
    results_file << elements[0] << ",";
    // Number of weeks
    results_file << elements[1] << ",";
    // Min handling
    results_file << elements[2] << ",";
    // Max handling
    results_file << elements[3] << ",";
    // Bunker price
    results_file << elements[4] << ",";
    // Penalty coefficient
    results_file << elements[5] << ",";
    // Min time window
    results_file << elements[6] << ",";
    // Max time window
    results_file << elements[7] << ",";
    // Min transit
    results_file << elements[8] << ",";
    // Max transit
    results_file << elements[9] << ",";
    
    results_file << ",";
    
    results_file << elapsed_time << ",";
    results_file << total_time_on_master << ",";
    results_file << total_time_on_pricing << ",";
    results_file << ub << ",";
    results_file << lb << ",";
    results_file << gap << ",";
    results_file << gap_at_root << ",";
    results_file << bb_nodes_generated << ",";
    results_file << max_depth << ",";
    results_file << pool->size() << ",";
    
    results_file << ",";
    
    // 1) Ships used
    auto vc_used = std::unordered_map<std::shared_ptr<VesselClass>, unsigned int>();
    auto actual_base = std::vector<Column>();
    
    for(const auto& vc : prob->data.vessel_classes) {
        vc_used[vc] = 0u;
    }
    
    if(node_bound_type == BoundType::FROM_LP) {
        for(const auto& col : node_attaining_ub->base_columns) {
            actual_base.push_back(col.first);
            vc_used[col.first.sol.vessel_class]++;
        }
    } else {
        for(const auto& col : node_attaining_ub->mip_base_columns) {
            actual_base.push_back(col.first);
            vc_used[col.first.sol.vessel_class]++;
        }
    }
    
    for(const auto& vc : prob->data.vessel_classes) {
        results_file << vc->name << "=" << vc_used[vc] << "/" << vc->num_vessels << " ";
    }
    
    results_file << ",";
    
    // 2) Average length of a rotation
    auto rot_lengths = 0.0;
    
    for(const auto& col : actual_base) {
        rot_lengths += col.sol.length();
    }
    
    results_file << rot_lengths / actual_base.size() << ",";
    
    // 3) Average travel distance of cargo
    
    auto distance_sum = 0.0;
    auto distance_n = 0u;
    
    for(const auto& col : actual_base) {
        auto d = col.sol.cargo_travel_distances();
        distance_sum += std::accumulate(d.begin(), d.end(), 0.0);
        distance_n += d.size();
    }
    
    results_file << (distance_sum / distance_n);
    
    results_file << std::endl;
    results_file.close();
}

void BBTree::print_row(const BBNode& current_node, double gap_node) const {
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

void BBTree::print_summary() const {
    auto all_penalties = 0.0;
    for(const auto& p : prob->data.ports) {
        all_penalties += p->pickup_penalty + p->delivery_penalty;
    }
    
    std::cout << std::endl << "*** SOLUTION ***" << std::endl;
    std::cout << "Total cost: " << ub << " (all penalties: " << all_penalties << ")" << std::endl;
        
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

void BBTree::branch_on_cycles(const Cycles& cycles, std::shared_ptr<BBNode> current_node) {    
    auto shortest = std::min_element(cycles.begin(), cycles.end(),
        [] (const auto& c1, const auto& c2) {
            return(c1.first.size() < c2.first.size());
        });

    const auto g = shortest->second;    

    std::cerr << "\t\tShortest cycle of length " << shortest->first.size() << " on graph for vc " << g->vessel_class->name << ": ";
    Cycle::print_cycle(shortest->first, g, std::cerr);

    for(auto fix_forb = shortest->first.begin(); fix_forb != shortest->first.end(); ++fix_forb) {
        std::cerr << "\t\t\tCreating child node:" << std::endl;
        
        VisitRuleList unite_rules, separate_rules;
        
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
                        
                        VisitRuleList unite_rules, separate_rules;
                        unite_rules.push_back(make_pair(n_src, n));
                        separate_rules.push_back(make_pair(n_src, n));
                        
                        std::cerr << "\t\t\tCreating child node 1:" << std::endl;
                        unexplored_nodes.push(
                            std::make_shared<BBNode>(
                                current_node->prob,
                                current_node->local_graphs,
                                current_node->pool,
                                current_node->local_pool,
                                unite_rules,
                                VisitRuleList(),
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
                                VisitRuleList(),
                                separate_rules,
                                current_node->sol_value,
                                current_node->depth + 1
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
    
    if(feasible_columns.size() <= (unsigned int) prob->params.max_cols_to_solve_mp) {
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
