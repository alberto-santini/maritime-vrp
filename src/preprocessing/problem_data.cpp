//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <stdexcept>

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "problem_data.h"

namespace mvrp {
    ProblemData::ProblemData(const std::string &data_file_name) {
        using namespace boost::property_tree;

        ptree pt;
        read_json(data_file_name, pt);

        num_ports = pt.get<int>("num_ports");
        num_times = pt.get<int>("num_time_intervals");
        num_vessel_classes = pt.get<int>("num_vessel_classes");

        std::vector<std::vector<double>> _distances;

        BOOST_FOREACH(const ptree::value_type &child, pt.get_child("vessel_classes")) {
                        auto name = child.second.get<std::string>("vessel_class_name");
                        auto num_vessels = child.second.get<int>("number_of_available_vessels");
                        auto capacity = child.second.get<int>("capacity_in_ffe");
                        auto time_charter_cost_per_time_unit = child.second.get<double>(
                            "time_charter_cost_per_time_interval");
                        auto hotel_cost_per_time_unit = child.second.get<double>(
                            "cost_when_idle_in_dollars_per_time_interval");

                        SpeedCostMap bunker_cost_per_time_unit;

                        std::vector<double> speeds;
                        std::vector<double> costs;
                        auto num_speeds = 0u;
                        auto num_speed_costs = 0u;

                        BOOST_FOREACH(const ptree::value_type &s_child,
                                      child.second.get_child("speeds_in_miles_per_time_interval")) {
                                        speeds.push_back(s_child.second.get<double>(""));
                                        num_speeds++;
                                    }

                        BOOST_FOREACH(const ptree::value_type &c_child,
                                      child.second.get_child("speed_costs_in_dollars_per_time_interval")) {
                                        costs.push_back(c_child.second.get<double>(""));
                                        num_speed_costs++;
                                    }

                        assert(num_speeds == num_speed_costs);

                        for(auto i = 0u; i < num_speeds; i++) bunker_cost_per_time_unit.emplace(speeds[i], costs[i]);

                        auto vessel_class = std::make_shared<VesselClass>(name, capacity, num_vessels,
                                                                          time_charter_cost_per_time_unit,
                                                                          hotel_cost_per_time_unit,
                                                                          bunker_cost_per_time_unit);
                        vessel_classes.push_back(vessel_class);
                    }

        if(vessel_classes.size() != (unsigned int) num_vessel_classes) {
            throw std::runtime_error("Declared more/less vessel classes than described");
        }

        // WARNING: THE CORRECT FUNCTIONING OF THE PROGRAMME RELIES ON THE FACT THAT IT EXPECTS THE HUB AS
        // THE FIRST PORT OF THE LIST, I.E. OF THE JSON ARRAY - JSON ARRAYS ARE GUARANTEED TO BE READ IN ORDER.

        BOOST_FOREACH(const ptree::value_type &child, pt.get_child("ports")) {
                        auto name = child.second.get<std::string>("unlo_code");
                        auto hub = child.second.get<bool>("is_hub");
                        auto pickup_demand = child.second.get<int>("pickup_demand_in_ffe");
                        auto delivery_demand = child.second.get<int>("delivery_demand_in_ffe");
                        auto pickup_transit = child.second.get<int>("pickup_max_transit_time_in_time_intervals");
                        auto delivery_transit = child.second.get<int>("delivery_max_transit_time_in_time_intervals");
                        auto pickup_handling = child.second.get<int>("pickup_handling_time_in_time_intervals");
                        auto delivery_handling = child.second.get<int>("delivery_handling_time_in_time_intervals");
                        auto num_tw = child.second.get<int>("number_of_time_windows");
                        auto pickup_movement_cost = child.second.get<double>("total_movement_cost_pickup");
                        auto delivery_movement_cost = child.second.get<double>("total_movement_cost_delivery");
                        auto fixed_fee = child.second.get<double>("call_fee_fixed_in_dollars");
                        auto pickup_revenue = child.second.get<double>("total_revenue_for_pickup");
                        auto delivery_revenue = child.second.get<double>("total_revenue_for_delivery");
                        auto pickup_penalty = child.second.get<double>("penalty_if_not_served_pickup");
                        auto delivery_penalty = child.second.get<double>("penalty_if_not_served_delivery");
                        std::vector<int> tw_left;
                        std::vector<int> tw_right;
                        std::vector<double> p_distances;
                        AllowedVcMap allowed;
                        VcFee variable_fee;
                        ClosingTimeWindows tw;

                        auto n = 0;
                        BOOST_FOREACH(const ptree::value_type &a_child,
                                      child.second.get_child("allowed_vessel_classes")) {
                                        allowed.emplace(vessel_classes[n++], a_child.second.get<bool>(""));
                                    }

                        n = 0;
                        BOOST_FOREACH(const ptree::value_type &f_child,
                                      child.second.get_child("call_fee_per_vessel_class_in_dollars")) {
                                        variable_fee.emplace(vessel_classes[n++], f_child.second.get<double>(""));
                                    }

                        BOOST_FOREACH(const ptree::value_type &l_child,
                                      child.second.get_child("time_windows_start_time_intervals")) {
                                        tw_left.push_back(l_child.second.get<int>(""));
                                    }

                        if(tw_left.size() != (unsigned int) num_tw) {
                            throw std::runtime_error("Declared more/less [left] time windows than described");
                        }

                        BOOST_FOREACH(const ptree::value_type &r_child,
                                      child.second.get_child("time_windows_end_time_intervals")) {
                                        tw_right.push_back(r_child.second.get<int>(""));
                                    }

                        if(tw_left.size() != (unsigned int) num_tw) {
                            throw std::runtime_error("Declared more/less [right] time windows than described");
                        }

                        for(auto i = 0; i < num_tw; i++) {
                            tw.push_back(std::make_pair(tw_left[i], tw_right[i]));
                        }

                        BOOST_FOREACH(const ptree::value_type &d_child, child.second.get_child("distances")) {
                                        p_distances.push_back(d_child.second.get<double>(""));
                                    }

                        if(p_distances.size() != (unsigned int) num_ports) {
                            throw std::runtime_error("Declared more/less distances than ports");
                        }

                        _distances.push_back(p_distances);

                        auto p = std::make_shared<Port>(name,
                                                        pickup_demand,
                                                        delivery_demand,
                                                        pickup_transit,
                                                        delivery_transit,
                                                        pickup_handling,
                                                        delivery_handling,
                                                        hub,
                                                        allowed,
                                                        pickup_movement_cost,
                                                        delivery_movement_cost,
                                                        fixed_fee,
                                                        variable_fee,
                                                        pickup_revenue,
                                                        delivery_revenue,
                                                        pickup_penalty,
                                                        delivery_penalty,
                                                        tw);

                        ports.push_back(p);
                    }

        if(ports.size() != (unsigned int) num_ports) {
            throw std::runtime_error("Declared more/less ports than described");
        }

        for(auto i = 0; i < num_ports; i++) {
            for(auto j = 0; j < num_ports; j++) {
                distances.emplace(std::make_pair(ports[i], ports[j]), _distances[i][j]);
            }
        }
    }

    std::vector<std::pair<std::shared_ptr<Port>, PickupType>> ProblemData::get_ports_list() const {
        auto list = std::vector<std::pair<std::shared_ptr<Port>, PickupType>>();

        for(auto p : ports) {
            list.push_back(std::make_pair(p, PickupType::PICKUP));
            list.push_back(std::make_pair(p, PickupType::DELIVERY));
        }

        return list;
    }
}