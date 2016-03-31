cmake_minimum_required(VERSION 2.8.4)
project(maritime_vrp)

# CPLEX ROOT DIR HINT
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(CPLEX_ROOT_DIR "/opt/ibm" CACHE FILEPATH "IBM Cplex install dir")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(CPLEX_ROOT_DIR "/opt/ibm/ILOG/CPLEX_Studio126" CACHE FILEPATH "IBM Cplex install dir")
endif()

# BOOST ROOT DIR HINT (Linux only)
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(BOOST_ROOT "~/boost")
endif()
# if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
#     set(Boost_NO_SYSTEM_PATHS TRUE)
#     set(BOOST_ROOT "/opt/boost-1.58.0-beta")
#     set(BOOST_INCLUDEDIR "/opt/boost-1.58.0-beta/include")
# endif()

# COMPILER: COMMON FLAGS
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DIL_STD -m64 -Wall -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=true -DPEDANTIC=true -DABO_BOOST_DEBUG=true -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -DDEBUG=false -DPEDANTIC=false -O3 -flto")

# COMPILER: PLATFORM-DEPENDENT FLAGS
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -save-temps=obj")
endif()

# LOAD MODULES: cplex, boost
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Cplex)
find_package(Boost)

set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem ")

# INCLUDE DIRECTORIES
include_directories("src")
include_directories(SYSTEM ${CPLEX_INCLUDE_DIRS})
include_directories(SYSTEM ${Boost_INCLUDE_DIR})

# BUILD THE EXECUTABLE INSIDE ./build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(SOURCE_FILES
  src/base/arc.h
  src/base/graph.h
  src/base/graph_properties.h
  src/base/node.h
  src/base/port.h
  src/base/problem.h
  src/base/vessel_class.h
  src/branching/bb_node.h
  src/branching/bb_tree.h
  src/branching/cycle.h
  src/column/column.h
  src/column/column_pool.h
  src/column/solution.h
  src/masterproblem/mp_integer_solution.h
  src/masterproblem/mp_linear_solution.h
  src/masterproblem/mp_solver.h
  src/preprocessing/graph_generator.h
  src/preprocessing/problem_data.h
  src/preprocessing/program_params.h
  src/subproblem/exact_solver.h
  src/subproblem/heuristics_solver.h
  src/subproblem/id_maps.h
  src/subproblem/labelling.h
  src/subproblem/sp_solver.h
  src/base/graph.cpp
  src/base/node.cpp
  src/base/problem.cpp
  src/base/vessel_class.cpp
  src/branching/bb_node.cpp
  src/branching/bb_tree.cpp
  src/branching/cycle.cpp
  src/column/column.cpp
  src/column/solution.cpp
  src/main.cpp
  src/masterproblem/mp_solver.cpp
  src/preprocessing/graph_generator.cpp
  src/preprocessing/problem_data.cpp
  src/preprocessing/program_params.cpp
  src/subproblem/exact_solver.cpp
  src/subproblem/heuristics_solver.cpp
  src/subproblem/labelling.cpp
  src/subproblem/sp_solver.cpp
)

add_executable(maritime_vrp ${SOURCE_FILES})
target_link_libraries(maritime_vrp ${CPLEX_LIBRARIES})