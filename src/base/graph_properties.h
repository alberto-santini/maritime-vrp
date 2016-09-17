#ifndef GRAPH_PROPERTIES_H
#define GRAPH_PROPERTIES_H

#include <memory>
#include <unordered_map>
#include <utility>

#include "port.h"
#include "vessel_class.h"

namespace mvrp {
    // TODO: turn the pair into a struct
    using PortDuals = std::unordered_map<std::shared_ptr<Port>, std::pair<double, double>>;
    using VcDuals = std::unordered_map<std::shared_ptr<VesselClass>, double>;

    struct GraphProperties {
        /**
         * Map that links each port to the duals for its pickup and delivery versions.
         */
        PortDuals port_duals;

        /**
         * Keeps a copy of the last port duals, before being modified.
         */
        PortDuals old_port_duals;

        /**
         * Dual value associated with the vessel class.
         */
        double vc_dual;

        GraphProperties() {}

        GraphProperties(PortDuals port_duals, PortDuals old_port_duals, double vc_dual) :
            port_duals(port_duals), old_port_duals(old_port_duals), vc_dual(vc_dual) {}
    };
}
#endif