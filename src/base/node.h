//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <memory>

#include "vessel_class.h"
#include "port.h"

namespace mvrp {
    enum class PortType {
        PICKUP, DELIVERY, BOTH
    };

    enum class NodeType {
        SOURCE_VERTEX, SINK_VERTEX, COMEBACK_HUB, REGULAR_PORT
    };

    struct Node {
        /**
         * Pointer to the associated port.
         */
        std::shared_ptr<Port> port;

        /**
         * Associated port type.
         */
        PortType pu_type;

        /**
         * Associated time step.
         */
        int time_step;

        /**
         * Node type: source, sink, or regular.
         */
        NodeType n_type;

        /**
         * Vessel class of the graph where the node resides.
         */
        std::shared_ptr<VesselClass> vessel_class;

        /**
         * Progressive id used by boost to identify the node.
         */
        int boost_vertex_id;

        Node() {}

        Node(std::shared_ptr<Port> port, PortType pu_type, NodeType n_type, int time_step, std::shared_ptr<VesselClass> vessel_class) :
            port(port), pu_type(pu_type), time_step(time_step), n_type(n_type), vessel_class(vessel_class)
        {
            boost_vertex_id = 0;
        }

        /**
         * Pickup demand of the node, if it is a pickup-node, or otherwise 0.
         */
        double pu_demand() const;

        /**
         * Delivery demand of the node, if it is a delivery node, or otherwise 0.
         */
        double de_demand() const;

        /**
         * Penalty to pay if service at the underlying port is skipped, if it is a pickup-node, otherwise 0.
         */
        double pu_penalty() const;

        /**
         * Penalty to pay if service at the underlying port is skipped, if it is a delivery-node, otherwise 0.
         */
        double de_penalty() const;

        /**
         * Penalty to pay if service at the underlying port is skipped, if it is a regular node, otherwise 0.
         */
        double penalty() const;

        /**
         * Two nodes are in the same row if they have the same port and port type (but possibly different time steps).
         * @param other Other node
         * @return      True iff the current and the other now are in the same row
         */
        bool same_row_as(const Node &other) const;

        /**
         * Two nodes are equal if they have the same port, port type, and time step.
         * @param other Other node
         * @return      True iff the current and the other nodes are equal
         */
        bool operator==(const Node &other) const;

        bool operator!=(const Node &other) const { return !(*this == other); }
    };

    /**
     * Prints a human-readable version of a port type.
     */
    std::ostream &operator<<(std::ostream &out, PortType pu);

    /**
     * Shortly prints info about a node.
     */
    std::ostream &operator<<(std::ostream &out, const Node &n);
}
#endif
