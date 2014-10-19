//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace boost::property_tree;

#include <preprocessing/program_params.h>

ProgramParams::ProgramParams(const std::string& data_file_name) {
    ptree pt;
    read_json(data_file_name, pt);
    
    delta = pt.get<int>("delta");
    theta = pt.get<int>("theta");
    max_cols_to_solve_mp = pt.get<int>("max_cols_to_solve_mp");
    dummy_column_price = pt.get<int>("dummy_column_price");
    remove_additional_arcs = pt.get<bool>("remove_additional_arcs");
    try_elementary_labelling = pt.get<bool>("try_elementary_labelling");
    try_smart_graph_reduction = pt.get<bool>("try_smart_graph_reduction");
    try_reduced_labelling = pt.get<bool>("try_reduced_labelling");
    smart_min_chance = pt.get<float>("smart_min_chance");
    smart_max_chance = pt.get<float>("smart_max_chance");
}