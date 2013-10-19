//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <branching/bb_node.h>

BBNode::BBNode(const std::shared_ptr<const Problem> prob, GraphMap local_graphs, const std::shared_ptr<ColumnPool> pool, const ColumnPool local_pool, const VisitRuleList unite_rules, const VisitRuleList separate_rules, const float father_lb, const IsolateRule isolate_rule) : prob(prob), local_graphs(local_graphs), pool(pool), local_pool(local_pool), unite_rules(unite_rules), separate_rules(separate_rules), father_lb(father_lb), isolate_rule(isolate_rule) {
    sol_value = numeric_limits<float>::max();
    mip_sol_value = numeric_limits<float>::max();
    make_local_graphs();
    remove_incompatible_columns();
}

void BBNode::make_local_graphs() {
    GraphMap new_graphs;
    for(const auto& vg : local_graphs) {
        std::shared_ptr<Graph> g = make_shared<Graph>(vg.second->graph, vg.second->vessel_class, vg.second->name);
        new_graphs.emplace(vg.first, g);
    }
    
    for(const VisitRule& vr : unite_rules) {
        std::shared_ptr<VesselClass> vc = vr.first->vessel_class;
        std::shared_ptr<Graph> g = new_graphs.at(vc);
        g->unite_ports(vr);
    }
    for(const VisitRule& vr : separate_rules) {
        std::shared_ptr<VesselClass> vc = vr.first->vessel_class;
        std::shared_ptr<Graph> g = new_graphs.at(vc);
        g->separate_ports(vr);
    }
    
    if(isolate_rule.first != nullptr && isolate_rule.second != nullptr) {
        for(auto& vg : new_graphs) {
            pair<vit, vit> vp;
            std::shared_ptr<Graph> g = vg.second;
            bool found_first = false;
            bool found_second = false;
        
            for(vp = vertices(g->graph); vp.first != vp.second; ++vp.first) {
                if(g->graph[*vp.first]->same_row_as(*isolate_rule.first) && !found_first) {
                    found_first = true;
                    g->isolate_port(g->graph[*vp.first]);
                    continue;
                }
                if(g->graph[*vp.first]->same_row_as(*isolate_rule.second) && !found_second) {
                    found_second = true;
                    g->isolate_port(g->graph[*vp.first]);
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
        
    for(const Column& c : local_pool) {
        bool compatible = true;
        for(const VisitRule& vr : unite_rules) {
            if(!c.is_compatible_with_unite_rule(vr)) {
                compatible = false;
                break;
            }
        }
        if(!compatible) {
            continue;
        }
        for(const VisitRule& vr : separate_rules) {
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

void BBNode::solve() {
    cerr << "\tGraphs at this node:" << endl;
    for(const auto& vg : local_graphs) {
        cerr << "\t\tVessel class: " << vg.first->name << endl;
        cerr << "\t\t\t" << vg.second->name << endl;
        cerr << "\t\t\t" << num_vertices(vg.second->graph) << " vertices" << endl;
        cerr << "\t\t\t" << num_edges(vg.second->graph) << " edges" << endl;
    }
    
    // Clear any eventual previous solutions
    base_columns = vector<pair<Column, float>>();
    sol_value = 0;
    
    MPSolver mp_solv(prob);
    MPLinearSolution sol = mp_solv.solve_lp(local_pool);
    cerr << unitbuf << "\tMP: " << sol.obj_value << " ";

    bool node_explored = false;
    while(!node_explored) {
        for(auto& vg : local_graphs) {
            vg.second->graph[graph_bundle].port_duals = sol.port_duals;
            vg.second->graph[graph_bundle].vc_dual = sol.vc_duals.at(vg.first);
        }

        SPSolver sp_solv(prob, local_graphs);
        int sp_found_columns;
        
        sp_found_columns = sp_solv.solve(local_pool, pool);

        if(sp_found_columns > 0) {
            // If new columns are found, solve the LP again
            sol = mp_solv.solve_lp(local_pool);
            cerr << "> " << sol.obj_value << " ";
        } else {
            // Otherwise, the exploration is done
            sol_value = sol.obj_value;

            for(int i = 0; i < sol.variables.size(); i++) {
                if(sol.variables[i] > BBNode::cplex_epsilon) {
                    base_columns.push_back(make_pair(local_pool[i], sol.variables[i]));
                }
            }

            node_explored = true;
        }
    }
    cerr << endl;
}

bool BBNode::solve_integer(const ColumnPool& feasible_columns) {
    // Clear any eventual previous solutions
    mip_base_columns = vector<pair<Column, float>>();
    mip_sol_value = 0;
    
    MPSolver mp_solv(prob);
    
    try {
        MPIntegerSolution sol = mp_solv.solve_mip(feasible_columns);
        mip_sol_value = sol.obj_value;

        for(int i = 0; i < sol.variables.size(); i++) {
            if(sol.variables[i] > BBNode::cplex_epsilon) {
                mip_base_columns.push_back(make_pair(feasible_columns[i], sol.variables[i]));
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