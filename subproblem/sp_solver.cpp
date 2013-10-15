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

int SPSolver::solve(ColumnPool& node_pool, std::shared_ptr<ColumnPool> global_pool) {
    vector<Solution> valid_sols;
    
    int discarded_prc = 0;
    int discarded_infeasible = 0;
    int discarded_generated = 0;
    int discarded_in_pool = 0;
    
    vector<std::shared_ptr<VesselClass>>::const_iterator vcit;
    
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
            Column col(prob, s, "fast heuristic");
            node_pool.push_back(col);
            global_pool->push_back(col);
        }
        return valid_sols.size();
    } else {
        discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    }
    
    cerr << "@";
    
    float lambda = prob->params.lambda_start;
    while(valid_sols.size() == 0 && lambda < prob->params.lambda_end + numeric_limits<float>::epsilon()) {
        for(vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
            const std::shared_ptr<Graph> g = local_graphs.at(*vcit);
            HeuristicsSolver hsolv(prob->params, g);
        
            vector<Solution> red_sols = hsolv.solve_on_reduced_graph(lambda);
    
            for(const Solution& s : red_sols) {
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
        lambda += prob->params.lambda_inc;
    }
    
    if(PEDANTIC) {
        cerr << "\t\t\tLabelling on the reduced graph." << endl;
        print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    }
    
    if(valid_sols.size() > 0) {
        for(const Solution& s : valid_sols) {
            Column col(prob, s, "labelling on the " + to_string(lambda - prob->params.lambda_inc) + "-reduced graph");
            node_pool.push_back(col);
            global_pool->push_back(col);
        }
        return valid_sols.size();
    } else {
        discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    }
    
    cerr << "@";
    
    for(vcit = prob->data.vessel_classes.begin(); vcit != prob->data.vessel_classes.end(); ++vcit) {
        const std::shared_ptr<Graph> g = local_graphs.at(*vcit);
        ExactSolver esolv(g);

        vector<Solution> e_sols = esolv.solve();
        
        for(const Solution& s : e_sols) {
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
        cerr << "\t\t\tLabelling on the complete graph." << endl;
        print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    }
       
    if(valid_sols.size() > 0) {
        for(const Solution& s : valid_sols) {
            Column col(prob, s, "labelling on the complete graph");
            node_pool.push_back(col);
            global_pool->push_back(col);
        }
        return valid_sols.size();
    }
    
    return 0;
}