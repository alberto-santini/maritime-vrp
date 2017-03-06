//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <iomanip>
#include <iostream>
#include <limits>

#include "column.h"

namespace mvrp {
    Column::Column(std::shared_ptr<const Problem> prob, const Solution &sol, ColumnOrigin origin) : prob(prob), sol(sol), origin(origin) {
        auto g = prob->graphs.at(sol.vessel_class);

        auto np = prob->data.num_ports;
        auto nv = prob->data.num_vessel_classes;

        /*  "np - 1" to remove the hub;
            "2 *" to create one coefficient for (port, pu) and one for (port, de) */
        port_coeff = std::vector<double>(2 * (np - 1), 0);
        for(const auto &e : sol.path) {
            Node n = *g->graph[target(e, g->graph)];
            if(n.n_type == NodeType::REGULAR_PORT) {
                for(auto i = 1; i < np; i++) {
                    if(n.port == prob->data.ports[i]) {
                        auto constr_index = (n.pu_type == PortType::PICKUP ? (i - 1) : (np - 1 + i - 1));
                        port_coeff[constr_index]++;
                    }
                }
            }
        }

        vc_coeff = std::vector<double>(nv, 0);
        for(auto i = 0; i < nv; i++) {
            if(sol.vessel_class == prob->data.vessel_classes[i]) {
                vc_coeff[i] = 1;
            }
        }

        obj_coeff = sol.cost;

        // Calculate penalties to remove from obj function:
        for(auto i = 1; i < np; ++i) {
            obj_coeff -= prob->data.ports[i]->pickup_penalty * port_coeff[i - 1];
            obj_coeff -= prob->data.ports[i]->delivery_penalty * port_coeff[np - 1 + i - 1];
        }

        dummy = false;
    }

    void Column::make_dummy(double huge_cost) {
        sol = Solution();
        obj_coeff = huge_cost;
        port_coeff = std::vector<double>(2 * (prob->data.num_ports - 1), 1);
        vc_coeff = std::vector<double>(prob->data.num_vessel_classes, 0);
        dummy = true;
        origin = ColumnOrigin::NONE;
    }

    bool Column::has_cycles() const {
        return std::any_of(port_coeff.begin(), port_coeff.end(), [](auto coeff) { return coeff > 1.000001; });
    }

    std::ostream &operator<<(std::ostream &out, const Column &c) {
        out << std::setw(6) << c.obj_coeff << " |\t";

        auto hs = (int) (c.port_coeff.size() / 2);

        for(auto i = 0; i < hs; i++) {
            out << c.port_coeff[i] << " " << c.port_coeff[hs + i] << "\t";
        }

        out << "| ";

        for(auto i = 0u; i < c.vc_coeff.size(); i++) {
            out << c.vc_coeff[i] << " ";
        }

        return out;
    }
}