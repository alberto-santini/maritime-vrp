//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include "node.h"

namespace mvrp {
    int Node::handling_time() const {
        if(n_type == NodeType::REGULAR_PORT) {
            if(pu_type == PortType::PICKUP) {
                return port->pickup_handling;
            } else if(pu_type == PortType::DELIVERY) {
                return port->delivery_handling;
            } else {
                return 0;
            }
        } else if(n_type == NodeType::COMEBACK_HUB) {
            return 2;
        } else {
            return 0;
        }
    }

    double Node::pu_demand() const {
        return (pu_type == PortType::PICKUP ? port->pickup_demand : 0);
    }

    double Node::de_demand() const {
        return (pu_type == PortType::DELIVERY ? port->delivery_demand : 0);
    }

    double Node::pu_penalty() const {
        return (pu_type == PortType::PICKUP ? port->pickup_penalty : 0);
    }

    double Node::de_penalty() const {
        return (pu_type == PortType::DELIVERY ? port->delivery_penalty : 0);
    }

    double Node::penalty() const {
        return (pu_type == PortType::PICKUP ? port->pickup_penalty : port->delivery_penalty);
    }

    bool Node::same_row_as(const Node &other) const {
        return (other.port == port && other.pu_type == pu_type);
    }

    bool Node::operator==(const Node &other) const {
        return (same_row_as(other) && other.time_step == time_step);
    }

    std::ostream &operator<<(std::ostream &out, PortType pu) {
        if(pu == PortType::PICKUP) { out << "pu"; }
        if(pu == PortType::DELIVERY) { out << "de"; }
        if(pu == PortType::BOTH) { out << "both"; }
        return out;
    }

    std::ostream &operator<<(std::ostream &out, const Node &n) {
        out << "[" << n.port->name << ", " << n.pu_type << ", " << n.time_step;
        out << ", dem: " << (n.pu_type == PortType::PICKUP ? n.pu_demand() : n.de_demand());
        out << "]";
        return out;
    }
}
