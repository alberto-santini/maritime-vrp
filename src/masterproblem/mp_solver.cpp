//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <stdexcept>
#include <string>
#include <vector>

#include "mp_solver.h"

namespace mvrp {
    MPSolver::IloData MPSolver::solve(const ColumnPool &pool, const std::vector<PortWithType>& ports_with_equality, bool linear, boost::optional<std::string> model_dump) const {
        // Calculate the sum of all penalties
        auto all_penalties = 0.0;
        for(const auto &p : prob->data.ports) {
            all_penalties += p->pickup_penalty + p->delivery_penalty;
        }

        IloEnv env;
        IloModel model(env);

        auto np = prob->data.num_ports;
        auto nv = prob->data.num_vessel_classes;
        std::stringstream cst_name;

        IloNumVarArray var(env);
        IloRangeArray port_constr(env, 2 * (np-1));
        IloRangeArray vc_constr(env);

        IloObjective obj = IloMinimize(env, all_penalties);

        // Port constraints
        for(auto i = 1; i < np; i++) {
            const auto port = prob->data.ports[i].get();

            bool pu_equality = std::find(
                ports_with_equality.begin(),
                ports_with_equality.end(),
                std::make_pair(port, PortType::PICKUP)) != ports_with_equality.end();

            bool de_equality = std::find(
                ports_with_equality.begin(),
                ports_with_equality.end(),
                std::make_pair(port, PortType::DELIVERY)) != ports_with_equality.end();

            try {
                cst_name << "port_" << port->name << "_pu";
                port_constr[i-1] = IloRange(env, pu_equality ? 1.0 : -IloInfinity, 1.0, cst_name.str().c_str());
                cst_name.str("");

                cst_name << "port_" << port->name << "_de";
                port_constr[np-1 + i-1] = IloRange(env, de_equality ? 1.0 : -IloInfinity, 1.0, cst_name.str().c_str());
                cst_name.str("");
            } catch(IloException& e) {
                std::cerr << "PortRows IloException: " << e << std::endl;
                throw;
            }
        }

        // Vessel constraints
        for(auto i = 0; i < nv; i++) {
            const auto& vc = *prob->data.vessel_classes[i];

            try {
                cst_name << "vc_" << vc.name;
                vc_constr.add(IloRange(env, -IloInfinity, prob->data.vessel_classes[i]->num_vessels, cst_name.str().c_str()));
                cst_name.str("");
            } catch(IloException& e) {
                std::cerr << "VCRows IloException: " << e << std::endl;
                throw;
            }
        }

        auto col_n = 0;
        for(auto cit = pool.begin(); cit != pool.end(); ++cit) {
            IloNumColumn ilo_c = obj(cit->obj_coeff);

            for(auto i = 1; i < np; i++) {
                try {
                    ilo_c += port_constr[i - 1](cit->port_coeff[i - 1]); // Pickup port
                    ilo_c += port_constr[np - 1 + i - 1](cit->port_coeff[np - 1 + i - 1]); // Delivery port
                } catch(IloException& e) {
                    std::cerr << "Cols PortRows IloExceptions: " << e << std::endl;
                    throw;
                }
            }
            for(auto i = 0; i < nv; i++) {
                try {
                    ilo_c += vc_constr[i](cit->vc_coeff[i]);
                } catch(IloException& e) {
                    std::cerr << "Cols VCRows IloExceptions: " << e << std::endl;
                    throw;
                }
            }

            IloNumVar v(ilo_c, 0, IloInfinity, (linear ? IloNumVar::Float : IloNumVar::Bool), ("theta_" + std::to_string(col_n++)).c_str());
            var.add(v);
        }

        model.add(obj);
        model.add(port_constr);
        model.add(vc_constr);

        IloCplex cplex(model);

        if(model_dump) {
            std::string output_name = *model_dump + ".lp";
            try {
                cplex.exportModel(output_name.c_str());
            } catch(IloException& e) {
                std::cerr << "Export IloException: " << e << std::endl;
            }
        }

        cplex.setParam(IloCplex::Threads, prob->params.cplex_cores);
        cplex.setOut(env.getNullStream());

        auto solved = false;

        try {
            solved = cplex.solve();
        } catch(IloException &e) {
            std::cerr << "Solve IloException: " << e << std::endl;
            throw;
        }

        if(!solved) {
            throw std::runtime_error("Infeasible problem!");
        }

        return std::make_tuple(env, var, port_constr, vc_constr, cplex);
    }

    MPLinearSolution MPSolver::solve_lp(const ColumnPool &pool, const std::vector<PortWithType>& ports_with_equality, boost::optional<std::string> model_dump) const {
        IloEnv env;
        IloNumVarArray var;
        IloRangeArray port_constr;
        IloRangeArray vc_constr;
        IloCplex cplex;

        auto np = prob->data.num_ports;

        std::tie(env, var, port_constr, vc_constr, cplex) = solve(pool, ports_with_equality, true, model_dump);

        auto obj_value = cplex.getObjValue();

        IloNumArray values(env);

        cplex.getDuals(values, port_constr);

        auto port_duals = PortDuals();
        for(auto i = 1; i <= (values.getSize() / 2); i++) {
            std::shared_ptr<Port> p = prob->data.ports[i];
            port_duals.emplace(p, std::make_pair(values[i - 1], values[np - 1 + i - 1]));
        }

        cplex.getDuals(values, vc_constr);

        auto vc_duals = VcDuals();
        for(auto i = 0; i < values.getSize(); i++) {
            std::shared_ptr<VesselClass> vc = prob->data.vessel_classes[i];
            vc_duals.emplace(vc, values[i]);
        }

        cplex.getValues(values, var);

        auto variables = std::vector<double>();
        for(auto i = 0; i < values.getSize(); i++) {
            variables.push_back(values[i]);
        }

        values.end();
        env.end();

        return MPLinearSolution(obj_value, port_duals, vc_duals, variables);
    }

    MPIntegerSolution MPSolver::solve_mip(const ColumnPool &pool, const std::vector<PortWithType>& ports_with_equality) const {
        IloEnv env;
        IloNumVarArray var;
        IloRangeArray port_constr;
        IloRangeArray vc_constr;
        IloCplex cplex;

        std::tie(env, var, port_constr, vc_constr, cplex) = solve(pool, ports_with_equality, false);

        auto obj_value = cplex.getObjValue();

        IloNumArray values(env);

        cplex.getValues(values, var);

        auto variables = std::vector<double>();
        for(auto i = 0; i < values.getSize(); i++) {
            variables.push_back(values[i]);
        }

        values.end();
        env.end();

        return MPIntegerSolution(obj_value, variables);
    }
}