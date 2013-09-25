#ifndef GRAPH_PROPERTIES_H
#define GRAPH_PROPERTIES_H

typedef std::unordered_map<std::shared_ptr<Port>, pair<float, float>> PortDuals;
typedef std::unordered_map<std::shared_ptr<VesselClass>, float> VcDuals;

class GraphProperties {
public:
    PortDuals                       port_duals;
    float                           vc_dual;
    int                             pu_upper_bound;
    int                             de_upper_bound;
    
    GraphProperties(const PortDuals port_duals = PortDuals(),
                    const float vc_dual = 0,
                    const int pu_upper_bound = 0,
                    const int de_upper_bound = 0) : port_duals(port_duals), vc_dual(vc_dual), pu_upper_bound(pu_upper_bound), de_upper_bound(de_upper_bound) {}
};

#endif