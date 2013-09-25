//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <subproblem/heuristics_solver.h>

vector<Solution> HeuristicsSolver::solve_fast_forward() const {
    vector<Solution> sols;
    Vertex h1 = g.h1().second;
    Vertex h2 = g.h2().second;
    struct EdgeWithCost { Edge e; float c; float rc; };

    srand(time(NULL) + 12345654321);

    for(int i = 0; i < params.theta; i++) {
        Vertex current = h1;
        Path path;
        float tot_c = 0, tot_rc = 0;
        bool done = false;
                
        while(current != h2) {
            vector<EdgeWithCost> out_e;
            
            pair<oeit, oeit> ep;
            for(ep = out_edges(current, g.graph); ep.first != ep.second; ++ep.first) {
                Node n_dest = *g.graph[target(*ep.first, g.graph)];
                float dual = g.dual_of(n_dest);
                EdgeWithCost ewc = { *ep.first, g.graph[*ep.first]->cost, g.graph[*ep.first]->cost - dual };
                
                bool closes_cycle = false;
                for(const Edge& e : path) {
                    if(n_dest.n_type == NodeType::REGULAR_PORT && n_dest.port == g.graph[source(e, g.graph)]->port) {
                        closes_cycle = true;
                        break;
                    }
                }
                
                if(!closes_cycle) {
                    out_e.push_back(ewc);
                }
            }
            
            if(out_e.size() == 0) {
                break;
            }
            
            sort(out_e.begin(), out_e.end(),
            [this] (const EdgeWithCost& ewc1, const EdgeWithCost& ewc2) {
                return (ewc1.rc > ewc2.rc);
            });
            
            int rnd_idx = rand() % (min(params.delta, (int)out_e.size()));
            EdgeWithCost chosen = out_e[rnd_idx];

            path.insert(path.begin(), chosen.e);
            tot_c += chosen.c;
            tot_rc += chosen.rc;
            
            if(path.size() > g.graph[graph_bundle].pu_upper_bound + g.graph[graph_bundle].de_upper_bound + 1) {
                break;
            }
            
            current = target(chosen.e, g.graph);
            
            if(current == h2) {
                done = true;
            }
        }
                
        if(done) {
            sols.push_back(Solution(path, tot_c, tot_rc, g.vessel_class));
        }
    }
    
    return sols;
}

vector<Solution> HeuristicsSolver::solve_fast_backward() const {
    vector<Solution> sols;
    Vertex h1 = g.h1().second;
    Vertex h2 = g.h2().second;
    struct EdgeWithCost { Edge e; float c; float rc; };

    srand(time(NULL) + 98765456789);

    for(int i = 0; i < params.theta; i++) {
        Vertex current = h2;
        Path path;
        float tot_c = 0, tot_rc = 0;
        bool done = false;
        
        while(current != h1) {
            vector<EdgeWithCost> in_e;
            
            pair<ieit, ieit> ep;
            for(ep = in_edges(current, g.graph); ep.first != ep.second; ++ep.first) {
                Node n_orig = *g.graph[source(*ep.first, g.graph)];
                float dual = g.dual_of(n_orig);
                EdgeWithCost ewc = { *ep.first, g.graph[*ep.first]->cost, g.graph[*ep.first]->cost - dual };
                
                bool closes_cycle = false;
                for(const Edge& e : path) {
                    if(n_orig.n_type == NodeType::REGULAR_PORT && n_orig.port == g.graph[target(e, g.graph)]->port) {
                        closes_cycle = true;
                        break;
                    }
                }
                
                if(!closes_cycle) {
                    in_e.push_back(ewc);
                }
            }
            
            if(in_e.size() == 0) {
                break;
            }
            
            sort(in_e.begin(), in_e.end(),
            [this] (const EdgeWithCost& ewc1, const EdgeWithCost& ewc2) {
                return (ewc1.rc > ewc2.rc);
            });
            
            int rnd_idx = rand() % (min(params.delta, (int)in_e.size()));
            EdgeWithCost chosen = in_e[rnd_idx];
            
            path.push_back(chosen.e);
            tot_c += chosen.c;
            tot_rc += chosen.rc;
            
            if(path.size() > g.graph[graph_bundle].pu_upper_bound + g.graph[graph_bundle].de_upper_bound + 1) {
                break;
            }
            
            current = source(chosen.e, g.graph);
            
            if(current == h1) {
                done = true;
            }
        }
        
        if(done) {
            sols.push_back(Solution(path, tot_c, tot_rc, g.vessel_class));
        }
    }
    
    return sols;
}

vector<Solution> HeuristicsSolver::solve_on_reduced_graph(const float lambda) const {
    vector<Solution> sols;
    Graph red;
    
    g.reduce_graph(lambda, red);
        
    vector<Path> optimal_paths;
    vector<Label> optimal_labels;
    
    NodeIdFunctor nf(red);
    ArcIdFunctor af(red);
    
    std::shared_ptr<VesselClass> vc = red.vessel_class;

    clock_t cl_start = clock();

    r_c_shortest_paths(
        red.graph,
        make_property_map<Vertex>(nf),
        make_property_map<Edge>(af),
        red.h1().second,
        red.h2().second,
        optimal_paths,
        optimal_labels,
        Label(vc->capacity, vc->capacity, 0, 0),
        LabelExtender(),
        Dominance(),
        allocator<r_c_shortest_paths_label<BGraph, Label>>(),
        default_r_c_shortest_paths_visitor()
    );
        
    clock_t cl_end = clock();
    // cout << "Time elapsed (on " << lambda << "-reduced graph, " << num_edges(red.graph) << " edges): " << (double(cl_end - cl_start) / CLOCKS_PER_SEC) << " seconds." << endl;
    
    for(int i = 0; i < optimal_paths.size(); i++) {
        Path og_path = g.transfer_path(optimal_paths[i], red);
        sols.push_back(Solution(og_path, g.calculate_cost(og_path), optimal_labels[i].cost, vc));
    }
        
    return sols;
}