//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>
#include <limits>
#include <numeric>

#include <branching/bb_node.h>

BBNode::BBNode(const std::shared_ptr<const Problem> prob, const GraphMap& local_graphs, const std::shared_ptr<ColumnPool> pool, const ColumnPool& local_pool, const VisitRuleList& unite_rules, const VisitRuleList& separate_rules, double father_lb, int depth, const IsolateRule& isolate_rule, bool try_elementary, double avg_time_spent_on_sp, double total_time_spent_on_sp, double total_time_spent_on_mp, double total_time_spent, double max_time_spent_by_exact_solver) : prob(prob), local_graphs(local_graphs), pool(pool), local_pool(local_pool), unite_rules(unite_rules), separate_rules(separate_rules), father_lb(father_lb), depth(depth), isolate_rule(isolate_rule), try_elementary(try_elementary), avg_time_spent_on_sp(avg_time_spent_on_sp), total_time_spent_on_sp(total_time_spent_on_sp), total_time_spent_on_mp(total_time_spent_on_mp), total_time_spent(total_time_spent), max_time_spent_by_exact_solver(max_time_spent_by_exact_solver) {
    sol_value = std::numeric_limits<double>::max();
    mip_sol_value = std::numeric_limits<double>::max();
    all_times_spent_on_sp = std::vector<double>(0);
    make_local_graphs();
    remove_incompatible_columns();
}

void BBNode::make_local_graphs() {
    GraphMap new_graphs;
    
    for(const auto& vg : local_graphs) {
        auto g = std::make_shared<Graph>(vg.second->graph, vg.second->vessel_class, vg.second->name);
        new_graphs.emplace(vg.first, g);
    }
    
    for(const auto& vr : unite_rules) {
        auto vc = vr.first->vessel_class;
        auto g = new_graphs.at(vc);
        g->unite_ports(vr);
    }
    for(const auto& vr : separate_rules) {
        auto vc = vr.first->vessel_class;
        auto g = new_graphs.at(vc);
        g->separate_ports(vr);
    }
    
    if(isolate_rule.first != nullptr && isolate_rule.second != nullptr) {
        for(const auto& vg : new_graphs) {
            auto g = vg.second;
            auto found_first = false;
            auto found_second = false;
        
            for(auto vp = vertices(g->graph); vp.first != vp.second; ++vp.first) {
                if(g->graph[*vp.first]->same_row_as(*isolate_rule.first) && !found_first) {
                    found_first = true;
                    g->isolate_port(*(g->graph[*vp.first]));
                    continue;
                }
                if(g->graph[*vp.first]->same_row_as(*isolate_rule.second) && !found_second) {
                    found_second = true;
                    g->isolate_port(*(g->graph[*vp.first]));
                    continue;
                }
                if(found_first && found_second) {
                    break;
                }
            }
        }
    }
    
    local_graphs = new_graphs;
}

void BBNode::remove_incompatible_columns() {
    ColumnPool new_local_pool;
        
    for(const auto& c : local_pool) {
        auto compatible = true;
        for(const auto& vr : unite_rules) {
            if(!c.is_compatible_with_unite_rule(vr)) {
                compatible = false;
                break;
            }
        }
        if(!compatible) {
            continue;
        }
        for(const auto& vr : separate_rules) {
            if(!c.is_compatible_with_separate_rule(vr)) {
                compatible = false;
                break;
            }
        }
        if(compatible) {
            new_local_pool.push_back(c);
        }
    }
    
    local_pool = new_local_pool;
}

std::vector<int> BBNode::column_coefficients(const Column& col) {
    auto np = prob->data.num_ports;
    auto nv = prob->data.num_vessel_classes;
    std::vector<int> column_coeff;
    
    for(auto i = 1; i < np; i++) {
        column_coeff.push_back((int)col.port_coeff[i - 1]);
        column_coeff.push_back((int)col.port_coeff[np - 1 + i - 1]);
    }
    for(auto i = 0; i < nv; i++) {
        column_coeff.push_back((int)col.vc_coeff[i]);
    }
    
    return column_coeff;
}

void BBNode::check_for_duplicate_columns() {
    for(auto oit = local_pool.begin(); oit != local_pool.end(); ++oit) {
        auto o_column_cost = oit->obj_coeff;
        auto o_column_coeff = column_coefficients(*oit);
        
        for(auto iit = oit + 1; iit != local_pool.end(); ++iit) {
            auto i_column_cost = iit->obj_coeff;
            auto i_column_coeff = column_coefficients(*iit);
            
            if(std::equal(o_column_coeff.begin(), o_column_coeff.end(), i_column_coeff.begin())) {
                if(fabs(o_column_cost - i_column_cost) < std::numeric_limits<double>::epsilon()) {
                    std::cerr << ">>> Duplicate column - same cost!" << std::endl;
                } else {
                    std::cerr << ">>> Duplicate column - different cost!" << std::endl;
                }
            }
        }
    }
}

void BBNode::solve() {
    auto node_start = clock();
    std::cerr << "\tGraphs at this node:" << std::endl;
    for(const auto& vg : local_graphs) {
        std::cerr << "\t\tVessel class: " << vg.first->name << std::endl;
        std::cerr << "\t\t\t" << vg.second->name << std::endl;
        std::cerr << "\t\t\t" << num_vertices(vg.second->graph) << " vertices" << std::endl;
        std::cerr << "\t\t\t" << num_edges(vg.second->graph) << " edges" << std::endl;
    }
    
    // Clear any eventual previous solutions
    base_columns = std::vector<std::pair<Column, double>>();
    sol_value = 0;
    auto mp_solv = MPSolver(prob);
    
    if(PEDANTIC) {
        check_for_duplicate_columns();
    }
    
    auto mp_start = clock();
    auto sol = mp_solv.solve_lp(local_pool);
    auto mp_end = clock();
    
    total_time_spent_on_mp += (double(mp_end - mp_start) / CLOCKS_PER_SEC);
    
    std::cerr << std::unitbuf << "\tMP: " << sol.obj_value << std::endl;

    auto node_explored = false;
    while(!node_explored) {
        for(const auto& vg : local_graphs) {
            vg.second->graph[graph_bundle].old_port_duals = vg.second->graph[graph_bundle].port_duals;
            vg.second->graph[graph_bundle].port_duals = sol.port_duals;
            vg.second->graph[graph_bundle].vc_dual = sol.vc_duals.at(vg.first);
        }

        auto sp_solv = SPSolver(prob, local_graphs);
        auto sp_found_columns = 0;
        auto orig = ColumnOrigin::NONE;
        
        // if(PEDANTIC) {
        //     std::cerr << "\t\tPort duals before solving SP:" << std::endl;
        //     for(const auto& pd : sol.port_duals) {
        //         std::cerr << "\t\t\t" << pd.first->name << "; pu: " << pd.second.first << ", de: " << pd.second.second << std::endl;
        //     }
        //     std::cerr << "\t\tVC duals before solving SP:" << std::endl;
        //     for(const auto& vd : sol.vc_duals) {
        //         std::cerr << "\t\t\t" << vd.first->name << ": " << vd.second << std::endl;
        //     }
        // }
        
        auto sp_start = clock();
        tie(sp_found_columns, orig) = sp_solv.solve(local_pool, pool, try_elementary, max_time_spent_by_exact_solver);
        auto sp_end = clock();
        
        total_time_spent_on_sp += (double(sp_end - sp_start) / CLOCKS_PER_SEC);
        all_times_spent_on_sp.push_back((double(sp_end - sp_start) / CLOCKS_PER_SEC));

        if(sp_found_columns > 0) {
            // If new columns are found, solve the LP again
            if((orig != ColumnOrigin::FAST_H) && (orig != ColumnOrigin::ESPPRC)) {
                try_elementary = false;
            }
            auto mp_start = clock();
            sol = mp_solv.solve_lp(local_pool);
            auto mp_end = clock();
            
            total_time_spent_on_mp += (double(mp_end - mp_start) / CLOCKS_PER_SEC);
            
            std::cerr << "\tMP: " << sol.obj_value << std::endl;
        } else {
            // Otherwise, the exploration is done
            sol_value = sol.obj_value;

            for(auto i = 0u; i < sol.variables.size(); i++) {
                if(sol.variables[i] > BBNode::cplex_epsilon) {
                    base_columns.push_back(std::make_pair(local_pool[i], sol.variables[i]));
                }
            }

            avg_time_spent_on_sp = std::accumulate(all_times_spent_on_sp.begin(), all_times_spent_on_sp.end(), 0.0) / all_times_spent_on_sp.size();
            node_explored = true;
        }
    }
    std::cerr << std::endl;
    auto node_end = clock();
    total_time_spent = (double(node_end - node_start) / CLOCKS_PER_SEC);
}

bool BBNode::solve_integer(const ColumnPool& feasible_columns) {
    // Clear any eventual previous solutions
    mip_base_columns = std::vector<std::pair<Column, double>>();
    mip_sol_value = 0;
    
    MPSolver mp_solv(prob);
    
    try {
        auto sol = mp_solv.solve_mip(feasible_columns);
        mip_sol_value = sol.obj_value;

        for(auto i = 0u; i < sol.variables.size(); i++) {
            if(sol.variables[i] > BBNode::cplex_epsilon) {
                mip_base_columns.push_back(std::make_pair(feasible_columns[i], sol.variables[i]));
            }
        }
        
        return true;
    } catch(...) {
        return false;
    }
}

bool BBNode::is_feasible() const {
    for(const auto& cc : base_columns) {
        if(cc.first.dummy) {
            return false;
        }
    }
    
    return true;
}

bool BBNode::is_integer_feasible() const {
    for(const auto& cc : mip_base_columns) {
        if(cc.first.dummy) {
            return false;
        }
    }
    
    return true;
}