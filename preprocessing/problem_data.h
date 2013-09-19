//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef PROBLEM_DATA_H
#define PROBLEM_DATA_H

#include <base/base.h>
#include <base/port.h>
#include <base/vessel_class.h>

/*  The third template parameter to unordered_map is the hash function to be used
    STL doesn't provide one for pair<>, so we use the one from boost */
typedef std::unordered_map<
            pair<std::shared_ptr<Port>, std::shared_ptr<Port>>, float,
            boost::hash<pair<std::shared_ptr<Port>,std::shared_ptr<Port>>>
        > DistMap;

class ProblemData {
public:
    int                                     num_ports;
    int                                     num_times;
    int                                     num_vessel_classes;
    vector<std::shared_ptr<VesselClass>>    vessel_classes;
    vector<std::shared_ptr<Port>>           ports;
    DistMap                                 distances;
    
    ProblemData(string data_file_name = "data/problem_data.json");
};

#endif
