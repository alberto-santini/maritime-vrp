//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <memory>

#include <base/vessel_class.h>
#include <base/port.h>

enum class PickupType { PICKUP, DELIVERY };
enum class NodeType { H1, H2, REGULAR_PORT };

class Node {
public:
    std::shared_ptr<Port>           port;
    PickupType                      pu_type;
    NodeType                        n_type;
    int                             time_step;
    std::shared_ptr<VesselClass>    vessel_class;
    int                             boost_vertex_id;
    
    Node() {}
    Node(std::shared_ptr<Port> port,
         PickupType pu_type,
         NodeType n_type,
         int time_step,
         std::shared_ptr<VesselClass> vessel_class) :
            port(port),
            pu_type(pu_type),
            n_type(n_type),
            time_step(time_step),
            vessel_class(vessel_class)
    {
        boost_vertex_id = 0;
    }
    
    double pu_demand() const;
    double de_demand() const;
    double pu_penalty() const;
    double de_penalty() const;
    double penalty() const;
    bool same_row_as(const Node& other) const;
    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const { return !(*this == other); }
};

std::ostream& operator<<(std::ostream& out, PickupType pu);
std::ostream& operator<<(std::ostream& out, const Node& n);
 
#endif
