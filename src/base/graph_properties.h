#ifndef GRAPH_PROPERTIES_H
#define GRAPH_PROPERTIES_H

#include <memory>
#include <unordered_map>
#include <utility>

#include <base/port.h>
#include <base/vessel_class.h>

typedef std::unordered_map<std::shared_ptr<Port>, std::pair<double, double>> PortDuals;
typedef std::unordered_map<std::shared_ptr<VesselClass>, double> VcDuals;

class GraphProperties {
public:
    PortDuals                       port_duals;
    PortDuals                       old_port_duals;
    double                          vc_dual;
    
    GraphProperties() {}
    
    GraphProperties(PortDuals port_duals,
                    PortDuals old_port_duals,
                    double vc_dual) : port_duals(port_duals), old_port_duals(old_port_duals), vc_dual(vc_dual) {}
};

#endif