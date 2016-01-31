//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <branching/bb_node.h>

#include <algorithm>
#include <limits>
#include <numeric>
#include <chrono>

BBNode::BBNode( std::shared_ptr<const Problem> prob,
                const ErasedEdgesMap& local_erased_edges,
                std::shared_ptr<ColumnPool> pool,
                const ColumnPool& local_pool,
                const VisitRuleList& unite_rules,
                const VisitRuleList& separate_rules,
                boost::optional<double> father_lb,
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
        local_erased_edges[vg.first] = vg.second->get_erased_edges_from_rules(local_erased_edges[vg.first], unite_rules, separate_rules);
    }
}

void BBNode::remove_incompatible_columns() {
    ColumnPool new_local_pool;

    for(const auto& c : local_pool) {
        if(std::any_of( unite_rules.begin(), unite_rules.end(),
                        [&] (const VisitRule& rule) { return !c.is_compatible_with_unite_rule(rule); }
        )) { continue; }
        
        if(std::any_of( separate_rules.begin(), separate_rules.end(),
                        [&] (const VisitRule& rule) { return !c.is_compatible_with_separate_rule(rule); }
        )) { continue; }
        
        new_local_pool.push_back(c);
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

void BBNode::remove_duplicate_columns() {
    ColumnPool new_pool;
    
    for(const auto& c : *pool) {
        auto nc = std::find_if( new_pool.begin(), new_pool.end(),
                                [&] (const Column& c_in_new_pool) {
                                    return column_coefficients(c) == column_coefficients(c_in_new_pool);
                                });

        if(nc == new_pool.end()) { new_pool.push_back(c); }
        else if(nc->obj_coeff > c.obj_coeff) { *nc = c; }
    }
    
    std::cerr   << "\tRemoved " << pool->size() - new_pool.size() << " duplicate columns"
                << " out of " << pool->size() << std::endl;
    
    *pool = new_pool;
}

void BBNode::solve(unsigned int node_number) {
    using namespace std::chrono;
    
    auto node_start = high_resolution_clock::now();
    
    // Clear any eventual previous solutions
    base_columns = std::vector<std::pair<Column, double>>();
    sol_value = 0;
    
    auto mp_solv = MPSolver(prob);
    
    // We start by solving the LP relaxation of the Master Problem
    auto mp_start = high_resolution_clock::now();
    auto sol = mp_solv.solve_lp(local_pool);
    auto mp_end = high_resolution_clock::now();
    
    auto mp_time = duration_cast<duration<double>>(mp_end - mp_start).count();
    total_time_spent_on_mp += mp_time;
    
    std::cerr   << std::unitbuf << "\tMP: " << std::setprecision(std::numeric_limits<double>::max_digits10)
                << sol.obj_value << " in " << std::setprecision(2) << mp_time << " seconds" << std::endl;
 
    auto node_explored = false;
    
    // Keep going until we can prove the node has been explored!
    while(!node_explored) {
        // Update dual values (and save the old port duals)
        for(const auto& vg : prob->graphs) {
            vg.second->graph[graph_bundle].old_port_duals = vg.second->graph[graph_bundle].port_duals;
            vg.second->graph[graph_bundle].port_duals = sol.port_duals;
            vg.second->graph[graph_bundle].vc_dual = sol.vc_duals.at(vg.first);
        }

        auto sp_solv = SPSolver(prob, local_erased_edges);
        auto sp_found_columns = 0;
        auto orig = ColumnOrigin::NONE;
        
        // Remember we only even try with elemntary labelling every n nodes
        if(node_number % prob->params.elementary_labelling_every_n_nodes != 0) { try_elementary = false; }
        
        // Solve the pricing subproblem
        auto sp_start = high_resolution_clock::now();
        tie(sp_found_columns, orig) = sp_solv.solve(local_pool, pool, try_elementary, max_time_spent_by_exact_solver);
        auto sp_end = high_resolution_clock::now();
        
        auto sp_time = duration_cast<duration<double>>(sp_end - sp_start).count();
        total_time_spent_on_sp += sp_time;
        all_times_spent_on_sp.push_back(sp_time);

        std::cerr   << std::unitbuf << "\t\tSP found " << sp_found_columns << " columns in "
                    << std::setprecision(2) << sp_time << " seconds" << std::endl;

        // If new [negative reduced cost] columns are found, solve the LP again
        if(sp_found_columns > 0) {
            // If:  we are currently trying elementary labelling AND
            //      at last iteration we tried elementary labelling AND
            //      it didn't produce any result
            // Then: stop trying elementary labelling
            if(try_elementary && (orig != ColumnOrigin::FAST_H) && (orig != ColumnOrigin::ESPPRC)) { try_elementary = false; }
            
            // Re-solve the LP
            auto mp_start = high_resolution_clock::now();
            sol = mp_solv.solve_lp(local_pool);
            auto mp_end = high_resolution_clock::now();
            
            total_time_spent_on_mp += duration_cast<duration<double>>(mp_end - mp_start).count();
            
            // If early branching is enabled and we are not at the root node
            if(prob->params.early_branching && father_lb) {
                auto time_at_node = duration_cast<duration<double>>(mp_end - node_start).count();
                
                // If it took too long to explore this node, perform early branching
                if(time_at_node > prob->params.early_branching_timeout) {
                    // Mark the node as explored (even if, technically, it is not)
                    node_explored = true;
                    
                    // We can't say anything on the LB at this node, so we have to carry the father's LB
                    sol_value = *father_lb;
                    
                    // We use the last solution to the LP as the solution to this node
                    for(auto i = 0u; i < sol.variables.size(); i++) {
                        if(sol.variables[i] > BBNode::cplex_epsilon) { base_columns.push_back(std::make_pair(local_pool[i], sol.variables[i])); }
                    }
                    
                    std::cerr << "Over early branching time limit: stopping the exploration of this node." << std::endl;
                    std::cerr << "Performing early branching with LB = Father's LB = " << sol_value << std::endl;
                }
            }
            
            std::cerr << std::unitbuf << "\tMP: " << std::setprecision(std::numeric_limits<double>::max_digits10) << sol.obj_value << std::endl;
        } else {
            // If no negative reduced cost column was found, the node is explored
            node_explored = true;
            
            // The [potentially fractional] solution at this node is the solution of the last LP solved
            sol_value = sol.obj_value;

            // Save the base columns of this solution
            for(auto i = 0u; i < sol.variables.size(); i++) {
                if(sol.variables[i] > BBNode::cplex_epsilon) { base_columns.push_back(std::make_pair(local_pool[i], sol.variables[i])); }
            }

            avg_time_spent_on_sp = std::accumulate(all_times_spent_on_sp.begin(), all_times_spent_on_sp.end(), 0.0) / all_times_spent_on_sp.size();
        }
    }
    
    auto node_end = high_resolution_clock::now();
    total_time_spent = duration_cast<duration<double>>(node_end - node_start).count();
    
    std::cerr << "Node explored in " << std::setprecision(2) << total_time_spent << " seconds" << std::endl;
    
    // Remove any duplicate column we might have generated at this node
    remove_duplicate_columns();
}

bool BBNode::solve_integer(const ColumnPool& feasible_columns) {
    // Clear any eventual previous solutions
    mip_base_columns = std::vector<std::pair<Column, double>>();
    mip_sol_value = 0;
    
    MPSolver mp_solv(prob);
    
    // Try to solve the integer problem with the columns we have at this node
    try {
        auto sol = mp_solv.solve_mip(feasible_columns);
        
        // Get the objective value of the MIP
        mip_sol_value = sol.obj_value;

        // And save the base columns of the MIP solution
        for(auto i = 0u; i < sol.variables.size(); i++) {
            if(sol.variables[i] > BBNode::cplex_epsilon) { mip_base_columns.push_back(std::make_pair(feasible_columns[i], sol.variables[i])); }
        }
        
        return true;
    } catch(...) {
        // If a [CPLEX] exception was thrown, then it was not possible to solve the MIP... sorry!
        return false;
    }
}

bool BBNode::is_feasible() const {
    // An LP solution is feasible if the dummy column is not in the base columns
    return std::none_of(base_columns.begin(), base_columns.end(),
                        [] (const auto& cc) { return cc.first.dummy; });
}

bool BBNode::is_integer_feasible() const {
    // A MIP solution is feasible if the dummy column is not in the base columns
    return std::none_of(mip_base_columns.begin(), mip_base_columns.end(),
                        [] (const auto& cc) { return cc.first.dummy; });
}