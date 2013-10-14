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

inline void print_report(const int sols_found, const int discarded_prc, const int discarded_infeasible, const int discarded_generated, const int discarded_in_pool) {
    cerr << "\t\t\t\tWe found " << sols_found << " new columns." << endl;
    cerr << "\t\t\t\t\t" << discarded_prc << " columns were discarded because they have positive reduced cost." << endl;
    cerr << "\t\t\t\t\t" << discarded_infeasible << " columns were discarded because they're infeasible wrt capacity constraints." << endl;
    cerr << "\t\t\t\t\t" << discarded_generated << " columns were discarded because they had already been generated in this iteration." << endl;
    cerr << "\t\t\t\t\t" << discarded_in_pool << " columns were discarded because they were already in the columns pool." << endl;
}

int SPSolver::solve(std::shared_ptr<ColumnPool> node_pool, std::shared_ptr<ColumnPool> global_pool) {
    std::shared_ptr<vector<Solution>> valid_sols = make_shared<vector<Solution>>();
    
    int discarded_prc = 0;
    int discarded_infeasible = 0;
    int discarded_generated = 0;
    int discarded_in_pool = 0;
    
    vector<std::shared_ptr<VesselClass>>::const_iterator vcit;
    vector<thread> threads;
    ThreadedSolver thr_sol;
    
    if(prob->params.num_cores < prob->data.vessel_classes.size()) {
        throw runtime_error("This CPU doesn't have enough cores to run all the labelling at the same time");
    }
    
    cerr << "@";
    
    for(vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
        const std::shared_ptr<Graph> g = local_graphs.at(*vcit);
        threads.push_back(thread([&] () {
            thr_sol.solve_fast(
            prob,
            g,
            node_pool,
            valid_sols,
            discarded_prc,
            discarded_infeasible,
            discarded_generated,
            discarded_in_pool);
        }));
    }
    
    for(auto& thr : threads) {
        thr.join();
    }
    
    // cout << "*** ALL THREADS FINISHED ***" << endl;
    // cout << "*** VALID_SOLS SIZE: " << valid_sols.size() << endl;
    
    if(PEDANTIC) {
        cerr << "\t\t\tFast heuristics." << endl;
        print_report(valid_sols->size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    }
    
    if(valid_sols->size() > 0) {
        for(const Solution& s : *valid_sols) {
            Column col(prob, s, "fast heuristic");
            node_pool->push_back(col);
            global_pool->push_back(col);
        }
        return valid_sols->size();
    } else {
        discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    }
    
    cerr << "@";
    
    float lambda = prob->params.lambda_start;
    while(valid_sols->size() == 0 && lambda < prob->params.lambda_end + numeric_limits<float>::epsilon()) {
        threads.clear();
        
        for(vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
            const std::shared_ptr<Graph> g = local_graphs.at(*vcit);
        
            threads.push_back(thread([&] () {
                thr_sol.solve_on_reduced_graph(
                prob,
                g,
                lambda,
                node_pool,
                valid_sols,
                discarded_prc,
                discarded_infeasible,
                discarded_generated,
                discarded_in_pool);
            }));
        }
        
        for(auto& thr : threads) {
            thr.join();
        }
        
        // cout << "*** ALL THREADS FINISHED ***" << endl;
        
        lambda += prob->params.lambda_inc;
    }
    
    if(PEDANTIC) {
        cerr << "\t\t\tLabelling on the reduced graph." << endl;
        print_report(valid_sols->size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    }
    
    if(valid_sols->size() > 0) {
        for(const Solution& s : *valid_sols) {
            Column col(prob, s, "labelling on the " + to_string(lambda - prob->params.lambda_inc) + "-reduced graph");
            node_pool->push_back(col);
            global_pool->push_back(col);
        }
        return valid_sols->size();
    } else {
        discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    }
    
    cerr << "@";
    
    threads.clear();
    for(vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
        const std::shared_ptr<Graph> g = local_graphs.at(*vcit);

        threads.push_back(thread([&] () {
            thr_sol.solve_on_complete_graph(
            g,
            node_pool,
            valid_sols,
            discarded_prc,
            discarded_infeasible,
            discarded_generated,
            discarded_in_pool);
        }));
    }
    
    for(auto& thr : threads) {
        thr.join();
    }

    if(PEDANTIC) {
        cerr << "\t\t\tLabelling on the complete graph." << endl;
        print_report(valid_sols->size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    }
       
    if(valid_sols->size() > 0) {
        for(const Solution& s : *valid_sols) {
            Column col(prob, s, "labelling on the complete graph");
            node_pool->push_back(col);
            global_pool->push_back(col);
        }
        return valid_sols->size();
    }
    
    return 0;
}