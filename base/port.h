//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef PORT_H
#define PORT_H

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <base/vessel_class.h>

typedef std::unordered_map<std::shared_ptr<VesselClass>, bool> AllowedVcMap;
typedef std::unordered_map<std::shared_ptr<VesselClass>, double> FeeVcMap;
typedef std::vector<std::pair<int, int>> ClosingTimeWindows;

class Port {
public:
    std::string         name;
    int                 pickup_demand;
    int                 delivery_demand;
    int                 pickup_transit;
    int                 delivery_transit;
    int                 pickup_handling;
    int                 delivery_handling;
    bool                hub;
    AllowedVcMap        allowed;
    FeeVcMap            fee;
    ClosingTimeWindows  closing_time_windows;
    
    Port() {}
    Port(const std::string& name,
         int pickup_demand,
         int delivery_demand,
         int pickup_transit,
         int delivery_transit,
         int pickup_handling,
         int delivery_handling,
         bool hub,
         const AllowedVcMap& allowed,
         const FeeVcMap& fee,
         const ClosingTimeWindows& closing_time_windows) : name(name), pickup_demand(pickup_demand), delivery_demand(delivery_demand), pickup_transit(pickup_transit), delivery_transit(delivery_transit), pickup_handling(pickup_handling), delivery_handling(delivery_handling), hub(hub), allowed(allowed), fee(fee), closing_time_windows(closing_time_windows) {}
};

#endif
