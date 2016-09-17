//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef PROBLEM_DATA_H
#define PROBLEM_DATA_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

#include "../base/node.h"
#include "../base/port.h"
#include "../base/vessel_class.h"

namespace mvrp {
    /*  The third template parameter to unordered_map is the hash function to be used
        STL doesn't provide one for pair<>, so we use the one from boost */
    using DistMap = std::unordered_map<
        std::pair<std::shared_ptr<Port>, std::shared_ptr<Port>>, double,
        boost::hash<std::pair<std::shared_ptr<Port>, std::shared_ptr<Port>>>>;

    class ProblemData {
    public:
        int num_ports;
        int num_times;
        int num_vessel_classes;
        std::vector<std::shared_ptr<VesselClass>> vessel_classes;
        std::vector<std::shared_ptr<Port>> ports;
        DistMap distances;

        ProblemData(const std::string &data_file_name);

        std::vector<std::pair<std::shared_ptr<Port>, PickupType>> get_ports_list() const;
    };
}

#endif
