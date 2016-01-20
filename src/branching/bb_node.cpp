//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>
#include <limits>
#include <numeric>

#include <branching/bb_node.h>

BBNode::BBNode( std::shared_ptr<const Problem> prob,
                const ErasedEdgesMap& local_erased_edges,
                std::shared_ptr<ColumnPool> pool,
                const ColumnPool& local_pool,
                const VisitRuleList& unite_rules,
                const VisitRuleList& separate_rules,
                double father_lb,
                int depth,
                bool try_elementary,
                double avg_time_spent_on_sp,
                double total_time_spent_on_sp,
                double total_time_spent_on_mp,
                double total_time_spent,
                double max_time_spent_by_exact_solver) :
                prob(prob),
                local_erased_edges(local_erased_edges),
                pool(pool),
                local_pool(local_pool),
                unite_rules(unite_rules),
                separate_rules(separate_rules),
                father_lb(father_lb),
                depth(depth),
                try_elementary(try_elementary),
                avg_time_spent_on_sp(avg_time_spent_on_sp),
                total_time_spent_on_sp(total_time_spent_on_sp),
                total_time_spent_on_mp(total_time_spent_on_mp),
                total_time_spent(total_time_spent),
                max_time_spent_by_exact_solver(max_time_spent_by_exact_solver)
{
    sol_value = std::numeric_limits<double>::max();
    mip_sol_value = std::numeric_limits<double>::max();
    all_times_spent_on_sp = std::vector<double>(0);
    make_local_erased_edges();
    remove_incompatible_columns();
}

void BBNode::make_local_erased_edges() {
    for(const auto& vg : prob->graphs) {
        local_erased_edges[vg.first] =
            vg.second->get_erased_edges_from_rules(local_erased_edges[vg.first], unite_rules, separate_rules);
    }
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
                if(fabs(o_column_cost - i_column_cost) < 0.00001) {
                    std::cerr << ">>> Duplicate column - same cost!" << std::endl;
                } else {
                    std::cerr << ">>> Duplicate column - different cost!" << std::endl;
                }
            }
        }
    }
}

void BBNode::solve(unsigned int node_number) {
    auto node_start = clock();
    
    // Clear any eventual previous solutions
    base_columns = std::vector<std::pair<Column, double>>();
    sol_value = 0;
    auto mp_solv = MPSolver(prob);
    
    // if(PEDANTIC) {
    //     check_for_duplicate_columns();
    // }
    
    auto mp_start = clock();
    auto sol = mp_solv.solve_lp(local_pool);
    auto mp_end = clock();
    
    total_time_spent_on_mp += (double(mp_end - mp_start) / CLOCKS_PER_SEC);
    
    std::cerr << std::unitbuf << "\tMP: " << sol.obj_value << std::endl;

    auto node_explored = false;
    while(!node_explored) {
        for(const auto& vg : prob->graphs) {
            vg.second->graph[graph_bundle].old_port_duals = vg.second->graph[graph_bundle].port_duals;
            vg.second->graph[graph_bundle].port_duals = sol.port_duals;
            vg.second->graph[graph_bundle].vc_dual = sol.vc_duals.at(vg.first);
        }

        auto sp_solv = SPSolver(prob, local_erased_edges);
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
        
        if(node_number % prob->params.elementary_labelling_every_n_nodes != 0) {
            try_elementary = false;
        }
        
        auto sp_start = clock();
        tie(sp_found_columns, orig) = sp_solv.solve(local_pool, pool, try_elementary, max_time_spent_by_exact_solver);
        auto sp_end = clock();
        
        total_time_spent_on_sp += (double(sp_end - sp_start) / CLOCKS_PER_SEC);
        all_times_spent_on_sp.push_back((double(sp_end - sp_start) / CLOCKS_PER_SEC));

        std::cerr << std::unitbuf << "\t\tSP found " << sp_found_columns << " columns" << std::endl;

        if(sp_found_columns > 0) {
            // If new columns are found, solve the LP again
            if(try_elementary && (orig != ColumnOrigin::FAST_H) && (orig != ColumnOrigin::ESPPRC)) {
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