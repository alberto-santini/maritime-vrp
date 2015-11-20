//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <algorithm>
#include <list>
#include <vector>

#include <boost/graph/r_c_shortest_paths.hpp>

#include <subproblem/heuristics_solver.h>

std::vector<Solution> HeuristicsSolver::solve_fast_forward() const {
    auto sols = std::vector<Solution>();
    auto h1 = g->h1().second;
    auto h2 = g->h2().second;
    struct EdgeWithCost { Edge e; double c; double rc; };

    for(auto i = 0; i < prob->params.theta; i++) {
        auto current = h1;
        auto path = Path();
        auto tot_c = 0.0f, tot_rc = 0.0f;
        auto done = false;
                
        while(current != h2) {
            std::vector<EdgeWithCost> out_e;
                        
            for(auto ep = out_edges(current, g->graph); ep.first != ep.second; ++ep.first) {
                auto n_dest = *g->graph[target(*ep.first, g->graph)];
                
                auto dual = g->dual_of(n_dest);
                EdgeWithCost ewc = { *ep.first, g->graph[*ep.first]->cost, g->graph[*ep.first]->cost - dual };
                
                auto closes_cycle = false;
                for(const auto& e : path) {
                    if(n_dest.n_type == NodeType::REGULAR_PORT && n_dest.port == g->graph[source(e, g->graph)]->port) {
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
            
            std::sort(out_e.begin(), out_e.end(),
            [this] (const auto& ewc1, const auto& ewc2) {
                return (ewc1.rc > ewc2.rc);
            });
            
            auto rnd_idx = (int) (rand() % (std::min(prob->params.delta, (int)out_e.size())));
            auto chosen = out_e[rnd_idx];

            path.insert(path.begin(), chosen.e);
            tot_c += chosen.c;
            tot_rc += chosen.rc;
            
            current = target(chosen.e, g->graph);
            
            if(current == h2) {
                done = true;
            }
        }
                
        if(done) {
            sols.push_back(Solution(path, tot_c, tot_rc, g->vessel_class, g));
        }
    }
    
    return sols;
}

std::vector<Solution> HeuristicsSolver::solve_fast_backward() const {
    std::vector<Solution> sols;
    auto h1 = g->h1().second;
    auto h2 = g->h2().second;
    struct EdgeWithCost { Edge e; double c; double rc; };

    for(int i = 0; i < prob->params.theta; i++) {
        auto current = h2;
        Path path;
        auto tot_c = 0.0f, tot_rc = 0.0f;
        auto done = false;
        
        while(current != h1) {
            std::vector<EdgeWithCost> in_e;
            
            for(auto ep = in_edges(current, g->graph); ep.first != ep.second; ++ep.first) {
                auto n_orig = *g->graph[source(*ep.first, g->graph)];
                auto dual = g->dual_of(n_orig);
                EdgeWithCost ewc = { *ep.first, g->graph[*ep.first]->cost, g->graph[*ep.first]->cost - dual };
                
                auto closes_cycle = false;
                for(const auto& e : path) {
                    if(n_orig.n_type == NodeType::REGULAR_PORT && n_orig.port == g->graph[target(e, g->graph)]->port) {
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
            
            std::sort(in_e.begin(), in_e.end(),
            [this] (const auto& ewc1, const auto& ewc2) {
                return (ewc1.rc > ewc2.rc);
            });
            
            auto rnd_idx = (int) (rand() % (std::min(prob->params.delta, (int)in_e.size())));
            auto chosen = in_e[rnd_idx];
            
            path.push_back(chosen.e);
            tot_c += chosen.c;
            tot_rc += chosen.rc;
            
            current = source(chosen.e, g->graph);
            
            if(current == h1) {
                done = true;
            }
        }
        
        if(done) {
            sols.push_back(Solution(path, tot_c, tot_rc, g->vessel_class, g));
        }
    }
    
    return sols;
}

std::vector<Solution> HeuristicsSolver::solve_fast() const {
    std::vector<Solution> fast_fwd_sols = solve_fast_forward();
    std::vector<Solution> fast_bwd_sols = solve_fast_backward();
    
    std::vector<Solution> total = fast_fwd_sols;
    total.insert(total.end(), fast_bwd_sols.begin(), fast_bwd_sols.end());
    
    return total;
}

std::vector<Solution> HeuristicsSolver::solve_elem_on_reduced_graph(double percentage) const {
    std::vector<Solution> sols;
    auto red = g->reduce_graph(percentage);
    
    std::vector<Path> optimal_paths;
    std::vector<ElementaryLabel> optimal_labels;
    
    NodeIdFunctor nf(red);
    ArcIdFunctor af(red);
    
    auto vc = red->vessel_class;
    VisitablePorts pf = prob->data.get_ports_list();
    
    r_c_shortest_paths(
        red->graph,
        make_property_map<Vertex>(nf),
        make_property_map<Edge>(af),
        red->h1().second,
        red->h2().second,
        optimal_paths,
        optimal_labels,
        ElementaryLabel(g, vc->capacity, vc->capacity, 0, pf),
        LabelExtender(),
        Dominance(),
        std::allocator<r_c_shortest_paths_label<BGraph, ElementaryLabel>>(),
        default_r_c_shortest_paths_visitor()
    );
            
    for(auto i = 0u; i < optimal_paths.size(); i++) {
        auto og_path = g->transfer_path(optimal_paths[i], *red);
        sols.push_back(Solution(og_path, g->calculate_cost(og_path), optimal_labels[i].cost, vc, red));
    }
        
    return sols;
}

std::vector<Solution> HeuristicsSolver::solve_on_generic_graph(double percentage, bool smart) const {
    std::vector<Solution> sols;
    auto red = smart ? g->smart_reduce_graph(prob->params.smart_min_chance, prob->params.smart_max_chance) : g->reduce_graph(percentage);
    
    std::vector<Path> optimal_paths;
    std::vector<Label> optimal_labels;
    
    NodeIdFunctor nf(red);
    ArcIdFunctor af(red);
    
    auto vc = red->vessel_class;

    r_c_shortest_paths(
        red->graph,
        make_property_map<Vertex>(nf),
        make_property_map<Edge>(af),
        red->h1().second,
        red->h2().second,
        optimal_paths,
        optimal_labels,
        Label(g, vc->capacity, vc->capacity, 0),
        LabelExtender(),
        Dominance(),
        std::allocator<r_c_shortest_paths_label<BGraph, Label>>(),
        default_r_c_shortest_paths_visitor()
    );
    
    for(auto i = 0u; i < optimal_paths.size(); i++) {
        auto og_path = g->transfer_path(optimal_paths[i], *red);
        sols.push_back(Solution(og_path, g->calculate_cost(og_path), optimal_labels[i].cost, vc, red));
    }
        
    return sols;
}