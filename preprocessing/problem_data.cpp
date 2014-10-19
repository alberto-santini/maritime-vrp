//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <stdexcept>

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace boost::property_tree;

#include <preprocessing/problem_data.h>

ProblemData::ProblemData(const std::string& data_file_name) {
    ptree pt;
    read_json(data_file_name, pt);
    
    num_ports = pt.get<int>("num_ports");
    num_times = pt.get<int>("num_times");
    num_vessel_classes = pt.get<int>("num_vessel_classes");
    
    std::vector<std::vector<float>> _distances;
    
    BOOST_FOREACH(const ptree::value_type& child, pt.get_child("vessel_classes")) {
        auto name = child.second.get<std::string>("name");
        auto num_vessels = child.second.get<int>("num_vessels");
        auto num_speeds = child.second.get<int>("num_speeds");
        auto capacity = child.second.get<int>("capacity");
        auto base_cost = child.second.get<float>("base_cost");
        SpeedCostMap bunker_cost;
        
        std::vector<float> speeds;
        std::vector<float> costs;
        
        BOOST_FOREACH(const ptree::value_type& s_child, child.second.get_child("speeds")) {
            speeds.push_back(s_child.second.get<float>(""));
        }
        
        BOOST_FOREACH(const ptree::value_type& c_child, child.second.get_child("speeds_cost")) {
            costs.push_back(c_child.second.get<float>(""));
        }
        
        if(speeds.size() != num_speeds || costs.size() != num_speeds) {
            throw std::runtime_error("Declared more/less speeds than described");
        }
        
        for(auto i = 0; i < num_speeds; i++) bunker_cost.emplace(speeds[i], costs[i]);
        
        auto vessel_class = std::make_shared<VesselClass>(name, capacity, num_vessels, base_cost, bunker_cost);
        vessel_classes.push_back(vessel_class);
    }
    
    if(vessel_classes.size() != num_vessel_classes) {
        throw std::runtime_error("Declared more/less vessel classes than described");
    }
    
    BOOST_FOREACH(const ptree::value_type& child, pt.get_child("ports")) {
        auto name = child.second.get<std::string>("name");
        auto hub = child.second.get<bool>("hub");
        auto pickup_demand = child.second.get<int>("pickup_dem");
        auto delivery_demand = child.second.get<int>("delivery_dem");
        auto pickup_transit = child.second.get<int>("pickup_trans");
        auto delivery_transit = child.second.get<int>("delivery_trans");
        auto pickup_handling = child.second.get<int>("pickup_hand");
        int delivery_handling = child.second.get<int>("delivery_hand");
        auto num_tw = child.second.get<int>("num_tw");
        std::vector<int> tw_left;
        std::vector<int> tw_right;
        std::vector<float> p_distances;
        AllowedVcMap allowed;
        FeeVcMap fee;
        ClosingTimeWindows tw;
        
        auto n = 0;
        BOOST_FOREACH(const ptree::value_type& a_child, child.second.get_child("allowed_vc")) {
            allowed.emplace(vessel_classes[n++], a_child.second.get<bool>(""));
        }
        
        n = 0;
        BOOST_FOREACH(const ptree::value_type& f_child, child.second.get_child("calling_fee")) {
            fee.emplace(vessel_classes[n++], f_child.second.get<float>(""));
        }
        
        BOOST_FOREACH(const ptree::value_type& l_child, child.second.get_child("tw_left")) {
            tw_left.push_back(l_child.second.get<int>(""));
        }
        
        if(tw_left.size() != num_tw) {
            throw std::runtime_error("Declared more/less [left] time windows than described");
        }
        
        BOOST_FOREACH(const ptree::value_type& r_child, child.second.get_child("tw_right")) {
            tw_right.push_back(r_child.second.get<int>(""));
        }
        
        if(tw_left.size() != num_tw) {
            throw std::runtime_error("Declared more/less [right] time windows than described");
        }
        
        for(auto i = 0; i < num_tw; i++) {
            tw.push_back(std::make_pair(tw_left[i], tw_right[i]));
        }
        
        BOOST_FOREACH(const ptree::value_type& d_child, child.second.get_child("distances")) {
            p_distances.push_back(d_child.second.get<float>(""));
        }
        
        if(p_distances.size() != num_ports) {
            throw std::runtime_error("Declared more/less distances than ports");
        }
        
        _distances.push_back(p_distances);
        
        auto p = std::make_shared<Port>(name, pickup_demand, delivery_demand, pickup_transit, delivery_transit, pickup_handling, delivery_handling, hub, allowed, fee, tw);
        
        ports.push_back(p);
    }
    
    if(ports.size() != num_ports) {
        throw std::runtime_error("Declared more/less ports than described");
    }
    
    for(auto i = 0; i < num_ports; i++) {
        for(auto j = 0; j < num_ports; j++) {
            distances.emplace(std::make_pair(ports[i], ports[j]), _distances[i][j]);
        }
    }
}