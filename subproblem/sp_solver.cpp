//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <subproblem/sp_solver.h>

inline bool solution_in_pool(const Solution& s, const ColumnPool& pool) {
    return (find_if(pool.begin(), pool.end(),
    [&s] (const Column& c) {
        return (s == c.sol);
    }) != pool.end());
}

inline void print_report(const int sols_found, const int discarded_prc, const int discarded_infeasible, const int discarded_generated, const int discarded_in_pool, ostream& out = cerr) {
    out << "\t\t\t\tWe found " << sols_found << " new columns." << endl;
    out << "\t\t\t\t\t" << discarded_prc << " columns were discarded because they have positive reduced cost." << endl;
    out << "\t\t\t\t\t" << discarded_infeasible << " columns were discarded because they're infeasible wrt capacity constraints." << endl;
    out << "\t\t\t\t\t" << discarded_generated << " columns were discarded because they had already been generated in this iteration." << endl;
    out << "\t\t\t\t\t" << discarded_in_pool << " columns were discarded because they were already in the columns pool." << endl;
}

pair<int, ColumnOrigin> SPSolver::solve(ColumnPool& node_pool, std::shared_ptr<ColumnPool> global_pool, const bool try_elementary) const {
    vector<Solution> valid_sols;
    
    int discarded_prc = 0;
    int discarded_infeasible = 0;
    int discarded_generated = 0;
    int discarded_in_pool = 0;
    
    vector<std::shared_ptr<VesselClass>>::const_iterator vcit;
    
    /********************** FAST HEURISTICS **********************/
    
    cerr << "@";
    
    for(vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {    
        const std::shared_ptr<Graph> g = local_graphs.at(*vcit);        
        HeuristicsSolver hsolv(prob->params, g);
        
        vector<Solution> total = hsolv.solve_fast();
                
        for(const Solution& s : total) {
            if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
                discarded_prc++;
            } else if(!s.satisfies_capacity_constraints()) {
                discarded_infeasible++;
            } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
                discarded_generated++;
            } else if(solution_in_pool(s, node_pool)) {
                discarded_in_pool++;
            } else {
                valid_sols.push_back(s);
            }
        }
    }
    
    if(PEDANTIC) {
        cerr << "\t\t\tFast heuristics." << endl;
        print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    }
    
    if(valid_sols.size() > 0) {
        for(const Solution& s : valid_sols) {
            Column col(prob, s, "fast heuristic", ColumnOrigin::FAST_H);
            node_pool.push_back(col);
            global_pool->push_back(col);
        }
        return make_pair(valid_sols.size(), ColumnOrigin::FAST_H);
    } else {
        discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    }
    
    float percentage = PCT_START;
    for(const std::shared_ptr<VesselClass>& vc : prob->data.vessel_classes) {
        local_graphs.at(vc)->sort_arcs();
    }
    
    /********************** ELEMENTARY LABELLING ON THE REDUCED GRAPH **********************/
    
    // if(try_elementary) {
    //     cerr << "@[";
    //
    //     while(valid_sols.size() == 0 && percentage < PCT_END_ELEM - numeric_limits<float>::epsilon()) {
    //         cerr << (int)(percentage * 10);
    //         std::shared_ptr<vector<Solution>> elem_sols = std::make_shared<vector<Solution>>();
    //         mutex mtx;
    //         vector<thread> threads;
    //     
    //         for(vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
    //             const std::shared_ptr<Graph> g = local_graphs.at(*vcit);
    //         
    //             threads.push_back(thread(
    //                 [this, g, &elem_sols, percentage, &mtx] () {                    
    //                     HeuristicsSolver hsolv(prob->params, g);         
    //                     vector<Solution> sols = hsolv.solve_elem_on_reduced_graph(percentage, prob);
    // 
    //                     lock_guard<mutex> guard(mtx);
    //                     elem_sols->insert(elem_sols->end(), sols.begin(), sols.end());
    //                 }
    //             ));
    //         }
    //     
    //         for(thread& t : threads) {
    //             t.join();
    //         }
    // 
    //         for(const Solution& s : *elem_sols) {
    //             if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
    //                 discarded_prc++;
    //             } else if(!s.satisfies_capacity_constraints()) {
    //                 discarded_infeasible++;
    //             } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
    //                 discarded_generated++;
    //             } else if(solution_in_pool(s, node_pool)) {
    //                 discarded_in_pool++;
    //             } else {
    //                 valid_sols.push_back(s);
    //             }
    //         }    
    //         percentage += PCT_INCREMENT;
    //     }
    //     cerr << "]";
    // 
    //     if(PEDANTIC) {
    //         cerr << "\t\t\tLabelling on the reduced graph." << endl;
    //         print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    //     }
    // 
    //     if(valid_sols.size() > 0) {
    //         for(const Solution& s : valid_sols) {
    //             Column col(prob, s, "ESPPRC", ColumnOrigin::ESPPRC);
    //             node_pool.push_back(col);
    //             global_pool->push_back(col);
    //         }
    //         return make_pair(valid_sols.size(), ColumnOrigin::ESPPRC);
    //     } else {
    //         discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    //     }
    // }
    
    /********************** LABELLING ON THE REDUCED GRAPH **********************/
    
    cerr << "@[";
    
    percentage = PCT_START;
    while(valid_sols.size() == 0 && percentage < PCT_END - numeric_limits<float>::epsilon()) {
        cerr << (int)(percentage * 10);
        std::shared_ptr<vector<Solution>> red_sols = std::make_shared<vector<Solution>>();
        mutex mtx;
        vector<thread> threads;
        
        for(vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
            const std::shared_ptr<Graph> g = local_graphs.at(*vcit);
            
            threads.push_back(thread(
                [this, g, &red_sols, percentage, &mtx] () {                    
                    HeuristicsSolver hsolv(prob->params, g);
                    vector<Solution> sols = hsolv.solve_on_reduced_graph(percentage);
    
                    lock_guard<mutex> guard(mtx);
                    red_sols->insert(red_sols->end(), sols.begin(), sols.end());
                }
            ));
        }
        
        for(thread& t : threads) {
            t.join();
        }
    
        for(const Solution& s : *red_sols) {
            if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
                discarded_prc++;
            } else if(!s.satisfies_capacity_constraints()) {
                discarded_infeasible++;
            } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
                discarded_generated++;
            } else if(solution_in_pool(s, node_pool)) {
                discarded_in_pool++;
            } else {
                valid_sols.push_back(s);
            }
        }    
        percentage += PCT_INCREMENT;
    }
    cerr << "]";
    
    if(PEDANTIC) {
        cerr << "\t\t\tLabelling on the reduced graph." << endl;
        print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    }
    
    if(valid_sols.size() > 0) {
        for(const Solution& s : valid_sols) {
            Column col(prob, s, "labelling on the " + to_string(percentage - PCT_INCREMENT) + "-reduced graph", ColumnOrigin::SPPRC_RED);
            node_pool.push_back(col);
            global_pool->push_back(col);
        }
        return make_pair(valid_sols.size(), ColumnOrigin::SPPRC_RED);
    } else {
        discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    }
    
    /********************** LABELLING ON THE COMPLETE GRAPH **********************/
    
    cerr << "@";
    
    std::shared_ptr<vector<Solution>> e_sols = std::make_shared<vector<Solution>>();
    mutex mtx;
    vector<thread> threads;
    
    for(vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
        const std::shared_ptr<Graph> g = local_graphs.at(*vcit);
        
        threads.push_back(thread(
            [this, g, &e_sols, &mtx] () {                    
                ExactSolver esolv(g);
                vector<Solution> sols = esolv.solve();

                lock_guard<mutex> guard(mtx);
                e_sols->insert(e_sols->end(), sols.begin(), sols.end());
            }
        ));
    }
    
    for(thread& t : threads) {
        t.join();
    }    
    
    for(const Solution& s : *e_sols) {
        if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
            discarded_prc++;
        } else if(!s.satisfies_capacity_constraints()) {
            discarded_infeasible++;
        } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
            discarded_generated++;
        } else if(solution_in_pool(s, node_pool)) {
            discarded_in_pool++;
        } else {
            valid_sols.push_back(s);
        }
    }

    if(PEDANTIC) {
        cerr << "\t\t\tLabelling on the complete graph." << endl;
        print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    }
       
    if(valid_sols.size() > 0) {
        for(const Solution& s : valid_sols) {
            Column col(prob, s, "labelling on the complete graph", ColumnOrigin::SPPRC);
            node_pool.push_back(col);
            global_pool->push_back(col);
        }
        return make_pair(valid_sols.size(), ColumnOrigin::SPPRC);
    }
    
    return make_pair(0, ColumnOrigin::NONE);
}