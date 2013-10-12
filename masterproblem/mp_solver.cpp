//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <masterproblem/mp_solver.h>

IloData MPSolver::solve(const ColumnPool& pool, const bool linear) const {
    IloEnv env;
    IloModel model(env);
    
    IloNumVarArray var(env);
    IloRangeArray port_constr(env);
    IloRangeArray vc_constr(env);
    
    IloObjective obj = IloMinimize(env);
    
    int np = prob->data.num_ports;
    int nv = prob->data.num_vessel_classes;
    
    for(int i = 1; i < np; i++) {
        port_constr.add(IloRange(env, 1.0, (linear ? IloInfinity : 1.0))); // Pickup port
        port_constr.add(IloRange(env, 1.0, (linear ? IloInfinity : 1.0))); // Delivery port
    }
    for(int i = 0; i < nv; i++) {
        vc_constr.add(IloRange(env, -IloInfinity, prob->data.vessel_classes[i]->num_vessels));
    }
    
    int col_n = 0;
    ColumnPool::const_iterator cit;
    for(cit = pool.begin(); cit != pool.end(); ++cit) {
        IloNumColumn ilo_c = obj(cit->obj_coeff);
        
        for(int i = 1; i < np; i++) {
            ilo_c += port_constr[i - 1](cit->port_coeff[i - 1]); // Pickup port
            ilo_c += port_constr[np - 1 + i - 1](cit->port_coeff[np - 1 + i - 1]); // Delivery port
        }
        for(int i = 0; i < nv; i++) {
            ilo_c += vc_constr[i](cit->vc_coeff[i]);
        }
        
        IloNumVar v(ilo_c, 0, 1, (linear ? IloNumVar::Float : IloNumVar::Bool), ("theta_" + to_string(col_n++)).c_str());
        var.add(v);
    }
    
    model.add(obj);
    model.add(port_constr);
    model.add(vc_constr);
    
    IloCplex cplex(model);
    cplex.setOut(env.getNullStream());
    
    if(!cplex.solve()) {
        throw runtime_error("Infeasible problem!");
    }
    
    return std::make_tuple(env, var, port_constr, vc_constr, cplex);
}

MPLinearSolution MPSolver::solve_lp(const ColumnPool& pool) const {
    IloEnv env;    
    IloNumVarArray var;
    IloRangeArray port_constr;
    IloRangeArray vc_constr;
    IloCplex cplex;
    
    int np = prob->data.num_ports;
    
    std::tie(env, var, port_constr, vc_constr, cplex) = solve(pool, true);
    
    float obj_value = cplex.getObjValue();
    
    IloNumArray values(env);
    cplex.getDuals(values, port_constr);
    // cout << "Port duals: " << values << endl;
    
    PortDuals port_duals;
    for(int i = 1; i <= (values.getSize() / 2); i++) {
        std::shared_ptr<Port> p = prob->data.ports[i];
        port_duals.emplace(p, make_pair(values[i - 1], values[np - 1 + i - 1]));
    }
    
    cplex.getDuals(values, vc_constr);
    // cout << "Vessel class duals: " << values << endl;
    
    VcDuals vc_duals;
    for(int i = 0; i < values.getSize(); i++) {
        std::shared_ptr<VesselClass> vc = prob->data.vessel_classes[i];
        vc_duals.emplace(vc, values[i]);
    }
    
    cplex.getValues(values, var);
    vector<float> variables;
    for(int i = 0; i < values.getSize(); i++) {
        variables.push_back(values[i]);
    }
    
    values.end();
    env.end();
    
    return MPLinearSolution(obj_value, port_duals, vc_duals, variables);
}

MPIntegerSolution MPSolver::solve_mip(const ColumnPool& pool) const {
    IloEnv env;    
    IloNumVarArray var;
    IloRangeArray port_constr;
    IloRangeArray vc_constr;
    IloCplex cplex;
    
    int np = prob->data.num_ports;
    
    std::tie(env, var, port_constr, vc_constr, cplex) = solve(pool, false);
    
    float obj_value = cplex.getObjValue();
    
    IloNumArray values(env);
    
    cplex.getValues(values, var);
    vector<float> variables;
    for(int i = 0; i < values.getSize(); i++) {
        variables.push_back(values[i]);
    }
    
    values.end();
    env.end();
    
    return MPIntegerSolution(obj_value, variables);
}