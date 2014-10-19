#ifndef GRAPH_PROPERTIES_H
#define GRAPH_PROPERTIES_H

#include <memory>
#include <unordered_map>
#include <utility>

#include <base/port.h>
#include <base/vessel_class.h>

typedef std::unordered_map<std::shared_ptr<Port>, std::pair<float, float>> PortDuals;
typedef std::unordered_map<std::shared_ptr<VesselClass>, float> VcDuals;

class GraphProperties {
public:
    PortDuals                       port_duals;
    PortDuals                       old_port_duals;
    float                           vc_dual;
    int                             pu_upper_bound;
    int                             de_upper_bound;
    
    GraphProperties() {}
    
    GraphProperties(PortDuals port_duals,
                    PortDuals old_port_duals,
                    float vc_dual,
                    int pu_upper_bound,
                    int de_upper_bound) : port_duals(port_duals), old_port_duals(old_port_duals), vc_dual(vc_dual), pu_upper_bound(pu_upper_bound), de_upper_bound(de_upper_bound) {}
};

#endif