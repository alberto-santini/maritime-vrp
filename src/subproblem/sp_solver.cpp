//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <thread>

#include <subproblem/sp_solver.h>

bool SPSolver::solution_in_pool(const Solution& s, const ColumnPool& pool) const {
    return (std::find_if(pool.begin(), pool.end(), [&s] (const Column& c) { return (s == c.sol); }) != pool.end());
}

void SPSolver::print_report(int sols_found, int discarded_prc, int discarded_infeasible, int discarded_generated, int discarded_in_pool, std::ostream& out) const {
    out << "\t\t\tWe found " << sols_found << " new columns." << std::endl;
    out << "\t\t\t\t" << discarded_prc << " columns were discarded because they have positive reduced cost." << std::endl;
    out << "\t\t\t\t" << discarded_infeasible << " columns were discarded because they're infeasible wrt capacity constraints." << std::endl;
    out << "\t\t\t\t" << discarded_generated << " columns were discarded because they had already been generated in this iteration." << std::endl;
    out << "\t\t\t\t" << discarded_in_pool << " columns were discarded because they were already in the columns pool." << std::endl;
}

std::pair<int, ColumnOrigin> SPSolver::solve(ColumnPool& node_pool, std::shared_ptr<ColumnPool> global_pool, bool try_elementary, double& max_time_spent_by_exact_solver) const {
    std::vector<Solution> valid_sols;
    
    auto discarded_prc = 0;
    auto discarded_infeasible = 0;
    auto discarded_generated = 0;
    auto discarded_in_pool = 0;
        
    /********************** FAST HEURISTICS **********************/
    
    if(PEDANTIC) { std::cerr << "\t\tFast heuristic" << std::endl; }
    
    for(auto vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {    
        auto g = prob->graphs.at(*vcit);
        const auto& erased = local_erased_edges.at(*vcit);
        
        HeuristicsSolver hsolv(prob, g, erased);
        
        auto total = hsolv.solve_fast();
                
        for(const auto& s : total) {
            if(s.reduced_cost > 0.0) {
                // if(PEDANTIC) {
                    // std::cerr << "\t\t\tDiscarded: reduced cost = " << s.reduced_cost << std::endl;
                    // g->print_path(s.path);
                // }
                discarded_prc++;
            } else if(!s.satisfies_capacity_constraints()) {
                discarded_infeasible++;
            } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
                discarded_generated++;
            } else if(solution_in_pool(s, node_pool)) {
                discarded_in_pool++;
            } else {
                // if(PEDANTIC) {
                    // std::cerr << "\t\t\tAccepted: reduced cost = " << s.reduced_cost << std::endl;
                // }
                valid_sols.push_back(s);
            }
        }
    }
    
    if(PEDANTIC) { print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool); }
    
    if(valid_sols.size() > 0) {
        for(const auto& s : valid_sols) {
            Column col(prob, s, "fast heuristic", ColumnOrigin::FAST_H);
            node_pool.push_back(col);
            global_pool->push_back(col);
        }
        return std::make_pair(valid_sols.size(), ColumnOrigin::FAST_H);
    } else {
        discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    }
    
    auto percentage = pct_start;
    for(const auto& vc : prob->data.vessel_classes) {
        prob->graphs.at(vc)->sort_arcs();
    }
    
    /********************** ELEMENTARY LABELLING ON THE REDUCED GRAPH **********************/
    
    if(prob->params.try_elementary_labelling && try_elementary) {
        if(PEDANTIC) { std::cerr << "\t\tElementary labelling"  << std::endl; }
        
        while(valid_sols.size() == 0 && percentage < pct_end_elem) {
            if(PEDANTIC) { std::cerr << "\t\t\t" << (int)(percentage * 100) << "%" << std::endl; }
            auto elem_sols = std::make_shared<std::vector<Solution>>();
            
            if(prob->params.parallel_labelling) {
                std::mutex mtx;
                std::vector<std::thread> threads;
        
                for(auto vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
                    std::shared_ptr<const Graph> g = prob->graphs.at(*vcit);
                    const auto& erased = local_erased_edges.at(*vcit);
            
                    threads.push_back(std::thread(
                        [this, g, &erased, &elem_sols, percentage, &mtx] () noexcept {                        
                            HeuristicsSolver hsolv(prob, g, erased);
                            auto sols = hsolv.solve_elem_on_reduced_graph(percentage);
    
                            std::lock_guard<std::mutex> guard(mtx);
                            elem_sols->insert(elem_sols->end(), sols.begin(), sols.end());
                        }
                    ));
                }
        
                for(auto& t : threads) {
                    t.join();
                }
            } else {
                for(auto vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
                    std::shared_ptr<const Graph> g = prob->graphs.at(*vcit);
                    const auto& erased = local_erased_edges.at(*vcit);
                    HeuristicsSolver hsolv(prob, g, erased);
                    auto sols = hsolv.solve_elem_on_reduced_graph(percentage);
                    elem_sols->insert(elem_sols->end(), sols.begin(), sols.end());
                }
            }
    
            for(const auto& s : *elem_sols) {
                if(s.reduced_cost > 0.0) {
                    // if(PEDANTIC) {
                        // std::cerr << "\t\t\tDiscarded: reduced cost = " << s.reduced_cost << std::endl;
                        // s.g->print_path(s.path);
                    // }
                    discarded_prc++;
                } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
                    discarded_generated++;
                } else if(solution_in_pool(s, node_pool)) {
                    discarded_in_pool++;
                } else {
                    // if(PEDANTIC) {
                        // std::cerr << "\t\t\tAccepted: reduced cost = " << s.reduced_cost << std::endl;
                    // }
                    valid_sols.push_back(s);
                }
                
                if(PEDANTIC) { assert(s.satisfies_capacity_constraints()); }
            }
            percentage += pct_increment;
        }
    
        if(PEDANTIC) { print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool); }
    
        if(valid_sols.size() > 0) {
            for(const auto& s : valid_sols) {
                Column col(prob, s, "ESPPRC", ColumnOrigin::ESPPRC);
                node_pool.push_back(col);
                global_pool->push_back(col);
            }
            return std::make_pair(valid_sols.size(), ColumnOrigin::ESPPRC);
        } else {
            discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
        }
    }
    
    /********************** LABELLING ON THE SMARTLY REDUCED GRAPH **********************/
    
    if(prob->params.try_smart_graph_reduction) {
        if(PEDANTIC) { std::cerr << "\t\tLabelling on the smartly reduced graph" << std::endl; }
        
        auto sred_sols = std::make_shared<std::vector<Solution>>();
        
        if(prob->params.parallel_labelling) {
            std::mutex mtx;
            std::vector<std::thread> threads;
    
            for(auto vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
                std::shared_ptr<const Graph> g = prob->graphs.at(*vcit);
                const auto& erased = local_erased_edges.at(*vcit);

                threads.push_back(std::thread(
                    [this, g, &erased, &sred_sols, &mtx] () noexcept {
                        HeuristicsSolver hsolv(prob, g, erased);
                        auto sols = hsolv.solve_on_smart_graph();
            
                        std::lock_guard<std::mutex> guard(mtx);
                        sred_sols->insert(sred_sols->end(), sols.begin(), sols.end());
                    }
                ));
            }
    
            for(auto& t : threads) {
                t.join();
            }
        } else {
            for(auto vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
                std::shared_ptr<const Graph> g = prob->graphs.at(*vcit);
                const auto& erased = local_erased_edges.at(*vcit);
                HeuristicsSolver hsolv(prob, g, erased);
                auto sols = hsolv.solve_on_smart_graph();
                sred_sols->insert(sred_sols->end(), sols.begin(), sols.end());
            }
        }
    
        for(const auto& s : *sred_sols) {
            if(s.reduced_cost > 0.0) {
                // if(PEDANTIC) {
                    // std::cerr << "\t\t\tDiscarded: reduced cost = " << s.reduced_cost << std::endl;
                    // s.g->print_path(s.path);
                // }
                discarded_prc++;
            } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
                discarded_generated++;
            } else if(solution_in_pool(s, node_pool)) {
                discarded_in_pool++;
            } else {
                // if(PEDANTIC) {
                    // std::cerr << "\t\t\tAccepted: reduced cost = " << s.reduced_cost << std::endl;
                // }
                valid_sols.push_back(s);
            }
            
            if(PEDANTIC) { assert(s.satisfies_capacity_constraints()); }
        }

        if(PEDANTIC) { print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool); }
       
        if(valid_sols.size() > 0) {
            for(const auto& s : valid_sols) {
                Column col(prob, s, "labelling on the smartly reduced graph", ColumnOrigin::SPPRC_SMART);
                node_pool.push_back(col);
                global_pool->push_back(col);
            }
            return std::make_pair(valid_sols.size(), ColumnOrigin::SPPRC_SMART);
        }
    }
    
    /********************** LABELLING ON THE REDUCED GRAPH **********************/
    
    if(prob->params.try_reduced_labelling) {
        if(PEDANTIC) { std::cerr << "\t\tLabelling on the reduced graph" << std::endl; }
        percentage = pct_start;
        
        while(valid_sols.size() == 0 && percentage < pct_end) {
            if(PEDANTIC) { std::cerr << "\t\t\t" << (int)(percentage * 100) << "%" << std::endl; }
            auto red_sols = std::make_shared<std::vector<Solution>>();
            
            if(prob->params.parallel_labelling) {
                std::mutex mtx;
                std::vector<std::thread> threads;
        
                for(auto vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
                    std::shared_ptr<const Graph> g = prob->graphs.at(*vcit);
                    const auto& erased = local_erased_edges.at(*vcit);
            
                    threads.push_back(std::thread(
                        [this, g, &erased, &red_sols, percentage, &mtx] () noexcept {                    
                            HeuristicsSolver hsolv(prob, g, erased);
                            auto sols = hsolv.solve_on_reduced_graph(percentage);
    
                            std::lock_guard<std::mutex> guard(mtx);
                            red_sols->insert(red_sols->end(), sols.begin(), sols.end());
                        }
                    ));
                }
        
                for(auto& t : threads) {
                    t.join();
                }
            } else {
                for(auto vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
                    std::shared_ptr<const Graph> g = prob->graphs.at(*vcit);
                    const auto& erased = local_erased_edges.at(*vcit);
                    HeuristicsSolver hsolv(prob, g, erased);
                    auto sols = hsolv.solve_on_reduced_graph(percentage);
                    red_sols->insert(red_sols->end(), sols.begin(), sols.end());
                }
            }
    
            for(const auto& s : *red_sols) {
                if(s.reduced_cost > 0.0) {
                    // if(PEDANTIC) {
                        // std::cerr << "\t\t\tDiscarded: reduced cost = " << s.reduced_cost << std::endl;
                        // s.g->print_path(s.path);
                    // }
                    discarded_prc++;
                } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
                    discarded_generated++;
                } else if(solution_in_pool(s, node_pool)) {
                    discarded_in_pool++;
                } else {
                    // if(PEDANTIC) {
                        // std::cerr << "\t\t\tAccepted: reduced cost = " << s.reduced_cost << std::endl;
                    // }
                    valid_sols.push_back(s);
                }
                
                if(PEDANTIC) { assert(s.satisfies_capacity_constraints()); }
            }    
            percentage += pct_increment;
        }
    
        if(PEDANTIC) { print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool); }
    
        if(valid_sols.size() > 0) {
            for(const auto& s : valid_sols) {
                Column col(prob, s, "labelling on the " + std::to_string(percentage - pct_increment) + "-reduced graph", ColumnOrigin::SPPRC_RED);
                node_pool.push_back(col);
                global_pool->push_back(col);
            }
            return std::make_pair(valid_sols.size(), ColumnOrigin::SPPRC_RED);
        } else {
            discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
        }
    }
    
    /********************** LABELLING ON THE COMPLETE GRAPH **********************/
    
    if(PEDANTIC) { std::cerr << "\t\tLabelling on the complete graph" << std::endl; }
    
    auto e_sols = std::make_shared<std::vector<Solution>>();
    
    if(prob->params.parallel_labelling) {
        std::mutex mtx;
        std::vector<std::thread> threads;
    
        for(auto vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
            std::shared_ptr<const Graph> g = prob->graphs.at(*vcit);
            const auto& erased = local_erased_edges.at(*vcit);
        
            threads.push_back(std::thread(
                [this, g, &erased, &e_sols, &mtx, &max_time_spent_by_exact_solver] () noexcept {                    
                    ExactSolver esolv(g, erased);
                    // Writing a double should be atomic on all x86_64 (-malign-double)
                    auto sols = esolv.solve(max_time_spent_by_exact_solver);

                    std::lock_guard<std::mutex> guard(mtx);
                    e_sols->insert(e_sols->end(), sols.begin(), sols.end());
                }
            ));
        }
    
        for(auto& t : threads) {
            t.join();
        }
    } else {
        for(auto vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
            std::shared_ptr<const Graph> g = prob->graphs.at(*vcit);
            const auto& erased = local_erased_edges.at(*vcit);
            ExactSolver esolv(g, erased);
            auto sols = esolv.solve(max_time_spent_by_exact_solver);
            e_sols->insert(e_sols->end(), sols.begin(), sols.end());
        }            
    }
    
    for(const auto& s : *e_sols) {
        if(s.reduced_cost > 0.0) {
            // if(PEDANTIC) {
                // std::cerr << "\t\t\tDiscarded: reduced cost = " << s.reduced_cost << std::endl;
                // s.g->print_path(s.path);
            // }
            discarded_prc++;
        } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
            discarded_generated++;
        } else if(solution_in_pool(s, node_pool)) {
            discarded_in_pool++;
        } else {
            // if(PEDANTIC) {
                // std::cerr << "\t\t\tAccepted: reduced cost = " << s.reduced_cost << std::endl;
            // }
            valid_sols.push_back(s);
        }
        
        if(PEDANTIC) { assert(s.satisfies_capacity_constraints()); }
    }

    if(PEDANTIC) {
        print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    }
       
    if(valid_sols.size() > 0) {
        for(const auto& s : valid_sols) {
            Column col(prob, s, "labelling on the complete graph", ColumnOrigin::SPPRC);
            node_pool.push_back(col);
            global_pool->push_back(col);
        }
        return std::make_pair(valid_sols.size(), ColumnOrigin::SPPRC);
    }
    
    return std::make_pair(0, ColumnOrigin::NONE);
}