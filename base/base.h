//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef BASE_H
#define BASE_H

#ifdef LOG
#define PEDANTIC true
#else
#define PEDANTIC false
#endif

// Boost Graph
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/r_c_shortest_paths.hpp>
// Boost Property Map
#include <boost/property_map/property_map.hpp>
// Boost Property Tree
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace boost::property_tree;
// Boost Other Utilities
#include <boost/utility/result_of.hpp>
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
using namespace boost;

// C Libraries
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstring>

// STD Library
#include <memory>
// std::shared_ptr could conflict with boost::
using std::make_shared;
using std::allocator;
#include <vector>
using std::vector;
#include <utility>
using std::pair;
using std::tie;
using std::make_pair;
#include <stdexcept>
using std::runtime_error;
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::setw;
using std::ostream;
using std::unitbuf;
using std::setprecision;
#include <limits>
using std::numeric_limits;
#include <algorithm>
using std::max;
using std::min;
using std::max_element;
using std::min_element;
using std::find;
using std::find_if;
using std::equal;
using std::sort;
using std::reverse;
using std::remove;
#include <string>
using std::string;
using std::to_string;
#include <unordered_map>
// std::unordered_map could confilct with boost::
#include <queue>
using std::priority_queue;
#include <tuple>
// std::tuple could conflict with boost::
using std::make_tuple;
using std::get;
#include <thread>
using std::thread;
#include <mutex>
using std::lock_guard;
using std::mutex;

// CPLEX
#include <ilcplex/ilocplex.h>

#endif
