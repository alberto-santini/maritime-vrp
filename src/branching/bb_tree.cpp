//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <numeric>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <fstream>
#include <limits>
#include <utility>
#include <chrono>
#include <iomanip>
#include <ctime>

#include "bb_tree.h"
#include "branching_rule.h"

namespace mvrp {
    BBTree::BBTree(const std::string &program_params_file_name, const std::string &data_file_name) : instance_file_name{data_file_name} {
        prob = std::make_shared<Problem>(program_params_file_name, data_file_name);
        ub = std::numeric_limits<double>::max();
        lb = std::numeric_limits<double>::max();

        Column dummy(prob);
        dummy.make_dummy(prob->params.dummy_column_price);
        pool = std::make_shared<ColumnPool>();
        pool->push_back(dummy);

        ErasedEdgesMap erased_edges{};
        for(auto vc : prob->data.vessel_classes) { erased_edges[vc] = ErasedEdges{}; }

        std::vector<PortWithType> ports_with_equality{};

        auto root_node = std::make_shared<BBNode>(prob, erased_edges, ports_with_equality, pool, *pool);

        unexplored_nodes.push(root_node);

        node_attaining_ub = root_node;
        node_bound_type = BoundType::FROM_LP;
        bb_nodes_generated = 1;
        elapsed_time = 0;
        max_depth = 0;
        total_time_on_master = 0;
        total_time_on_pricing = 0;
    }

    void BBTree::update_lb(std::shared_ptr<BBNode> current_node, unsigned int node_number) {
        // Root node. Global LB is node's LB.
        if(node_number <= 1u) {
            lb = current_node->sol_value;
            return;
        }

        // Ran out of nodes => Tree explored completely => Solution found! => LB = UB
        if(unexplored_nodes.empty()) {
            lb = ub;
            return;
        }

        lb = *unexplored_nodes.top()->father_lb;
    }

    void BBTree::explore_tree() {
        using namespace std::chrono;

        print_header();

        auto node_number = 0u;
        auto start_time = high_resolution_clock::now();

        while(!unexplored_nodes.empty()) {
            std::cerr << "Nodes in tree: " << unexplored_nodes.size() << std::endl;

            auto current_node = unexplored_nodes.top();
            unexplored_nodes.pop();

            current_node->solve(node_number++);
            std::cerr << "\tNode LB: " << std::setprecision(std::numeric_limits<double>::max_digits10)
                      << current_node->sol_value << std::endl;

            if(current_node->depth > max_depth) { max_depth = current_node->depth; }

            if(!current_node->is_feasible()) {
                std::cerr << "\t\tPruned by infeasibility (optimal LP solution contains dummy column)" << std::endl;

                if(node_number == 1u) { std::cout << "Root node infeasible" << std::endl; }

                update_lb(current_node, node_number);
                gap = std::abs((ub - lb) / ub) * 100;

                continue;
            }

            if(current_node->sol_value >= ub) {
                std::cerr << "\t\tPruned by sub-optimality (UB = " << ub << ")" << std::endl;

                update_lb(current_node, node_number);
                gap = std::abs((ub - lb) / ub) * 100;

                continue;
            }

            try_to_obtain_ub(current_node);

            if(current_node->has_fractional_solution() || current_node->has_solution_with_cycles()) {
                branch(current_node);
            };

            update_lb(current_node, node_number);

            auto gap_node = std::abs((ub - current_node->sol_value) / ub) * 100;
            gap = std::abs((ub - lb) / ub) * 100;

            if(node_number == 1u) { gap_at_root = gap_node; }

            total_time_on_master += current_node->total_time_spent_on_mp;
            total_time_on_pricing += current_node->total_time_spent_on_sp;

            print_row(*current_node, gap_node);

            auto curr_time = high_resolution_clock::now();
            auto el_time = duration_cast<duration<double>>(curr_time - start_time).count();

            if(el_time > prob->params.time_limit_in_s) {
                std::cerr << std::endl << "Over time limit! " << el_time << std::endl;
                break;
            }
        }

        auto end_time = high_resolution_clock::now();
        elapsed_time = duration_cast<duration<double>>(end_time - start_time).count();

        print_summary();
        print_results();
    }

    void BBTree::branch(std::shared_ptr<BBNode> current_node) {
        branch_on_port_selection(current_node) ||
        branch_on_vessel_assignment(current_node) ||
        branch_on_port_reached_from_two_ports(current_node) ||
        branch_on_speed(current_node) ||
        branch_on_arc(current_node);
    }

    bool BBTree::branch_on_port_selection(std::shared_ptr<BBNode> current_node) {
        using PortFlowMap = std::map<PortWithType, float>;
        PortFlowMap map;
        auto eps = 0.00001f;

        for(const auto& cc : current_node->base_columns) {
            if(cc.first.dummy) { continue; }

            const auto& column = cc.first;
            const auto& coefficient = cc.second;
            const auto& graph = column.sol.g;

            for(const auto& edge : column.sol.path) {
                const auto& v_orig = boost::source(edge, graph->graph);
                const auto& n_orig = graph->graph[v_orig];

                if(n_orig->n_type == NodeType::REGULAR_PORT) {
                    auto key = std::make_pair(n_orig->port.get(), n_orig->pu_type);
                    auto key_val_it = map.find(key);

                    if(key_val_it == map.end()) { map[key] = 0.0f; }

                    map[key] += coefficient;
                }
            }
        }

        Port* most_fractional_port = nullptr;
        PortType most_fractional_pu = PortType::BOTH;
        float most_fractional_val = .5f;

        for(const auto& kv : map) {
            const auto& key = kv.first;
            const auto& val = kv.second;

            if(val > eps && val < 1.0f - eps) {
                if(std::fabs(val - .5f) < most_fractional_val - eps) {
                    most_fractional_port = key.first;
                    most_fractional_pu = key.second;
                    most_fractional_val = std::fabs(val - .5f);
                }
            }
        }

        if(!most_fractional_port) { return false; }

        assert(most_fractional_pu != PortType::BOTH);

        std::shared_ptr<BranchingRule> include_port = std::make_shared<IncludePort>(most_fractional_port, most_fractional_pu);
        std::shared_ptr<BranchingRule> exclude_port = std::make_shared<ExcludePort>(most_fractional_port, most_fractional_pu);

        std::stringstream portname;
        portname << *most_fractional_port << " " << most_fractional_pu;

        std::cerr << "Branch on port visit: " << portname.str() << std::endl;

        if(PEDANTIC) {
            std::cerr << "The port has total in-flow of: " << most_fractional_val << std::endl;
            std::cerr << "Columns visiting the port:" << std::endl;
            for(const auto& cc : current_node->base_columns) {
                if(cc.first.sol.visits_port(*most_fractional_port, most_fractional_pu)) {
                    std::cerr << "\t" << cc.first << "\t(" << cc.second << ")" << std::endl;
                }
            }
        }

        std::cerr << "Branching from node: " << current_node->name << std::endl;

        unexplored_nodes.push(std::make_shared<BBNode>(*current_node, include_port, current_node->name + " (PV " + portname.str() + ")"));
        unexplored_nodes.push(std::make_shared<BBNode>(*current_node, exclude_port, current_node->name + " (!PV " + portname.str() + ")"));

        bb_nodes_generated += 2;

        return true;
    }

    bool BBTree::branch_on_vessel_assignment(std::shared_ptr<BBNode> current_node) {
        using PortVesselFlowMap = std::map<std::tuple<Port*, PortType, VesselClass*>, float>;
        PortVesselFlowMap map;
        auto eps = 0.00001f;

        for(const auto& cc : current_node->base_columns) {
            if(cc.first.dummy) { continue; }

            const auto& column = cc.first;
            const auto& coefficient = cc.second;
            const auto& graph = column.sol.g;
            const auto& vc = column.sol.vessel_class;

            for(const auto& edge : column.sol.path) {
                const auto& v_orig = boost::source(edge, graph->graph);
                const auto& n_orig = graph->graph[v_orig];

                if(n_orig->n_type == NodeType::REGULAR_PORT) {
                    auto key = std::make_tuple(n_orig->port.get(), n_orig->pu_type, vc.get());
                    auto key_val_it = map.find(key);

                    if(key_val_it == map.end()) { map[key] = 0.0f; }

                    map[key] += coefficient;
                }
            }
        }

        Port* most_fractional_port = nullptr;
        PortType most_fractional_pu = PortType::BOTH;
        VesselClass* most_fractional_vc = nullptr;
        float most_fractional_val = .5f;

        for(const auto& kv : map) {
            const auto& key = kv.first;
            const auto& val = kv.second;

            if(val > eps && val < 1.0f - eps) {
                if(std::fabs(val - .5f) < most_fractional_val - eps) {
                    most_fractional_port = std::get<0>(key);
                    most_fractional_pu = std::get<1>(key);
                    most_fractional_vc = std::get<2>(key);
                    most_fractional_val = std::fabs(val - .5f);
                }
            }
        }

        if(!most_fractional_port) {
            assert(!most_fractional_vc);
            return false;
        }

        assert(most_fractional_pu != PortType::BOTH);
        assert(most_fractional_vc);

        std::shared_ptr<BranchingRule> assign_port_v = std::make_shared<AssignToVessel>(most_fractional_port, most_fractional_pu, most_fractional_vc);
        std::shared_ptr<BranchingRule> forbid_port_v = std::make_shared<ForbidToVessel>(most_fractional_port, most_fractional_pu, most_fractional_vc);

        std::stringstream name;
        name << *most_fractional_port << " " << most_fractional_pu << "[" << *most_fractional_vc << "]";

        std::cerr << "Branch on port assignment to vessel: " << name.str() << std::endl;
        std::cerr << "Branching from node: " << current_node->name << std::endl;

        unexplored_nodes.push(std::make_shared<BBNode>(*current_node, assign_port_v, current_node->name + " (PA " + name.str() + ")"));
        unexplored_nodes.push(std::make_shared<BBNode>(*current_node, forbid_port_v, current_node->name + " (!PA " + name.str() + ")"));

        bb_nodes_generated += 2;

        return true;
    }

    bool BBTree::branch_on_port_reached_from_two_ports(std::shared_ptr<BBNode> current_node) {
        float most_fractional_val = .5f;
        VesselClass* most_fractional_vc = nullptr;
        auto eps = 0.00001f;

        // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
        boost::optional<std::pair<PortWithType, PortWithType>> most_fractional_port_succ = 
            boost::make_optional(false, std::pair<PortWithType, PortWithType>{});

        for(auto outer_it = current_node->base_columns.begin(); outer_it != current_node->base_columns.end(); ++outer_it) {
            if(outer_it->first.dummy) { continue; }

            const auto& outer_sol = outer_it->first.sol;
            const auto& outer_coeff = outer_it->second;

            if(std::fabs(outer_coeff - .5f) > most_fractional_val - eps) { continue; }

            // Check for a cycle in this column
            auto with_cycle = false;
            auto visited = outer_sol.visited_ports_with_predecessors();
            for(const auto& port_pred : visited) {
                if(port_pred.second.size() > 0) {
                    most_fractional_port_succ = std::make_pair(port_pred.second.back(), port_pred.first);
                    most_fractional_vc = outer_sol.vessel_class.get();
                    most_fractional_val = outer_coeff;
                    with_cycle = true;
                    break;
                }
            }

            if(!with_cycle) {
                // Check for another column which visit a port covered by this column, with a different predecessor
                for(auto inner_it = outer_it + 1; inner_it < current_node->base_columns.end(); ++inner_it) {
                    const auto& inner_sol = inner_it->first.sol;

                    auto port_succ = outer_sol.common_port_visited_from_two_different_predecessors(inner_sol);

                    if(port_succ) {
                        most_fractional_port_succ = port_succ;
                        most_fractional_vc = outer_sol.vessel_class.get();
                        most_fractional_val = outer_coeff;
                        break;
                    }
                }
            }
        }

        if(most_fractional_port_succ) {
            assert(most_fractional_vc);

            std::shared_ptr<BranchingRule> force_visit = std::make_shared<ForceConsecutiveVisit>(*most_fractional_port_succ, most_fractional_vc);
            std::shared_ptr<BranchingRule> forbid_visit = std::make_shared<ForbidConsecutiveVisit>(*most_fractional_port_succ, most_fractional_vc);

            std::stringstream name;
            name << *(most_fractional_port_succ->first.first) << " "
                 << most_fractional_port_succ->first.second << " - "
                 << *(most_fractional_port_succ->second.first) << " "
                 << most_fractional_port_succ->second.second;

            std::cerr << "Branch on consecutive visit: " << name.str() << std::endl;
            std::cerr << "Branching from node: " << current_node->name << std::endl;

            unexplored_nodes.push(std::make_shared<BBNode>(*current_node, force_visit, current_node->name + " (PS " + name.str() + ")"));
            unexplored_nodes.push(std::make_shared<BBNode>(*current_node, forbid_visit, current_node->name + " (!PS " + name.str() + ")"));

            bb_nodes_generated += 2;

            return true;
        }

        return false;
    }

    bool BBTree::branch_on_speed(std::shared_ptr<BBNode> current_node) {
        float most_fractional_val = .5f;
        VesselClass* most_fractional_vc = nullptr;
        auto eps = 0.00001f;

        // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
        boost::optional<std::tuple<PortWithType, PortWithType, double>> most_fractional_port_succ_speed =
            boost::make_optional(false, std::tuple<PortWithType, PortWithType, double>{});

        for(auto outer_it = current_node->base_columns.begin(); outer_it != current_node->base_columns.end(); ++outer_it) {
            if(outer_it->first.dummy) { continue; }

            const auto& outer_sol = outer_it->first.sol;
            const auto& outer_coeff = outer_it->second;

            if(std::fabs(outer_coeff - .5f) > most_fractional_val - eps) { continue; }

            for(auto inner_it = outer_it + 1; inner_it < current_node->base_columns.end(); ++inner_it) {
                const auto& inner_sol = inner_it->first.sol;

                auto port_succ = outer_sol.common_port_succession_at_two_different_speeds(inner_sol);

                if(port_succ) {
                    most_fractional_port_succ_speed = port_succ;
                    most_fractional_vc = outer_sol.vessel_class.get();
                    most_fractional_val = outer_coeff;
                    break;
                }
            }
        }

        if(most_fractional_port_succ_speed) {
            assert(most_fractional_vc);

            std::shared_ptr<BranchingRule> force_speed = std::make_shared<ForceSpeed>(*most_fractional_port_succ_speed, most_fractional_vc);
            std::shared_ptr<BranchingRule> forbid_speed = std::make_shared<ForbidSpeed>(*most_fractional_port_succ_speed, most_fractional_vc);

            std::cerr << "Branch on speed: "
                      << *(std::get<0>(*most_fractional_port_succ_speed).first) << " "
                      << std::get<0>(*most_fractional_port_succ_speed).second << " -> "
                      << *(std::get<1>(*most_fractional_port_succ_speed).first) << " "
                      << std::get<1>(*most_fractional_port_succ_speed).second << " at speed "
                      << std::get<2>(*most_fractional_port_succ_speed) << std::endl;
            std::cerr << "Branching from node: " << current_node->name << std::endl;

            unexplored_nodes.push(std::make_shared<BBNode>(*current_node, force_speed, current_node->name + " S"));
            unexplored_nodes.push(std::make_shared<BBNode>(*current_node, forbid_speed, current_node->name + " !S"));

            bb_nodes_generated += 2;

            return true;
        }

        return false;
    }

    bool BBTree::branch_on_arc(std::shared_ptr<BBNode> current_node) {
        float val = .5f;
        Column const* col = nullptr;
        auto eps = 0.00001f;

        for(const auto& cc : current_node->base_columns) {
            if(cc.first.dummy) { continue; }

            if(std::fabs(cc.second - .5f) < val - eps) {
                col = &cc.first;
                val = cc.second;
                break;
            }
        }

        assert(col);

        // Using make_optional because of a GCC -Wmaybe-uninitialized false positive:
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
        boost::optional<Edge> e = boost::make_optional(false, Edge{});

        for(auto edge : col->sol.path) {
            auto srcv = boost::source(edge, col->sol.g->graph);
            auto trgv = boost::target(edge, col->sol.g->graph);
            const auto& srcn = col->sol.g->graph[srcv];
            const auto& trgn = col->sol.g->graph[trgv];

            if(srcn->n_type == NodeType::REGULAR_PORT && trgn->n_type == NodeType::REGULAR_PORT) {
                e = edge;
                break;
            }
        }

        assert(e);

        std::shared_ptr<BranchingRule> force_arc = std::make_shared<ForceArc>(*e, col->sol.vessel_class.get());
        std::shared_ptr<BranchingRule> forbid_arc = std::make_shared<ForbidArc>(*e, col->sol.vessel_class.get());

        std::cerr << "Branch on arc for vessel " << *(col->sol.vessel_class) << std::endl;
        std::cerr << "Branching from node: " << current_node->name << std::endl;

        unexplored_nodes.push(std::make_shared<BBNode>(*current_node, force_arc, current_node->name + " A"));
        unexplored_nodes.push(std::make_shared<BBNode>(*current_node, forbid_arc, current_node->name + " !A"));

        bb_nodes_generated += 2;

        return true;
    }

    void BBTree::try_to_obtain_ub(std::shared_ptr<BBNode> current_node) {
        ColumnPool feasible_columns;

        for(const auto& c : current_node->local_pool) {
            if(!c.dummy && !c.has_cycles()) {
                feasible_columns.push_back(Column(c.prob, c.sol, ColumnOrigin::MIP));
            }
        }

        if(current_node->has_fractional_solution()) {
            if(
                current_node->depth == 0 ||
                feasible_columns.size() <= (unsigned int) prob->params.max_cols_to_solve_mp
            ) {
                if(current_node->solve_integer(feasible_columns)) {
                    std::cerr << "\tNode UB: " << std::setprecision(std::numeric_limits<double>::max_digits10)
                              << current_node->mip_sol_value << std::endl;
                    if(ub - current_node->mip_sol_value > BBNode::cplex_epsilon) {
                        std::cerr << "\t\tImproving the UB" << std::endl;
                        ub = current_node->mip_sol_value;
                        node_attaining_ub = current_node;
                        node_bound_type = BoundType::FROM_MIP;
                        for(const auto& cc : node_attaining_ub->mip_base_columns) {
                            std::cerr << "\t\t\t " << cc.first << " with coeff " << cc.second << std::endl;
                        }
                    }
                } else {
                    std::cerr << "\tMIP infeasible" << std::endl;
                }
            } else {
                std::cerr << "\tToo many columns to solve MIP" << std::endl;
            }
        } else {
            if(std::all_of(current_node->base_columns.begin(), current_node->base_columns.end(),
                [&] (const auto& cc) -> bool { return !cc.first.dummy && !cc.first.has_cycles(); }
            )) {
                if(ub - current_node->sol_value > BBNode::cplex_epsilon) {
                    std::cerr << "\t\tImproving the UB" << std::endl;
                    ub = current_node->sol_value;
                    node_attaining_ub = current_node;
                    node_bound_type = BoundType::FROM_LP;
                    for(const auto& cc : node_attaining_ub->base_columns) {
                        std::cerr << "\t\t\t " << cc.first << " with coeff " << cc.second << std::endl;
                    }
                }
            }
        }
    }

    void BBTree::print_header() const {
        std::cout << std::setw(22) << "BB Nodes |";
        std::cout << std::setw(28) << "Lower Bound |";
        std::cout << std::setw(16) << "Upper Bound |";
        std::cout << std::setw(28) << "Gap |";
        std::cout << std::setw(10) << "|";
        std::cout << std::setw(66) << "Total time |";
        std::cout << std::setw(22) << "|";
        std::cout << std::setw(26) << "|";
        std::cout << std::setw(8) << "|" << std::endl;

        std::cout << std::setw(12) << "Unexplored  "; // Unexplored nodes
        std::cout << std::setw(10) << "Total |"; // Number of nodes in total

        std::cout << std::setw(14) << "LB at node  ";
        std::cout << std::setw(14) << "LB best |";

        std::cout << std::setw(16) << "UB best |";

        std::cout << std::setw(14) << "Gap at node  ";
        std::cout << std::setw(14) << "Gap best |";

        std::cout << std::setw(10) << "Columns |";

        std::cout << std::setw(22) << "Time spent on MP  "; // Time on MP
        std::cout << std::setw(22) << "Time spent on SP  "; // Time on SP
        std::cout << std::setw(22) << "Time spent at node |"; // Time at node

        std::cout << std::setw(22) << "Average time on SP |"; // Avg time on SP
        std::cout << std::setw(26) << "Max time on SP (exact) |"; // Max time on Exact SP

        std::cout << std::setw(8) << "Depth |" << std::endl;

        std::cout << "-----------*---------*" <<
                  "-------------*-------------*" <<
                  "---------------*" <<
                  "-------------*-------------*" <<
                  "---------*" <<
                  "---------------------*---------------------*---------------------*" <<
                  "---------------------*-------------------------*" <<
                  "-------*" << std::endl;
    }

    // LIBSTD of GCC DOES NOT IMPLEMENT STD::DEFAULTFLOAT !!
    inline std::ostream &defaultfloat(std::ostream &os) {
        os.unsetf(std::ios_base::floatfield);
        return os;
    }

    void BBTree::print_results() const {
        std::ofstream results_file;
        results_file.open("results.txt", std::ios::out | std::ios::app);

        auto elements = std::vector<std::string>();
        auto ss = std::stringstream(instance_file_name);
        auto item = std::string();

        while(std::getline(ss, item, '/')) {
            elements.push_back(item);
        }

        // Remove .json
        for(auto i = 0u; i < 5u; ++i) { elements.back().pop_back(); }

        ss = std::stringstream(std::string(elements.back()));
        elements = std::vector<std::string>();
        item = std::string();

        while(std::getline(ss, item, '_')) {
            elements.push_back(item);
        }

        assert(elements.size() == 13u);

        // Scenario name
        results_file << elements[0] << ",";
        // Discretisation
        results_file << elements[1] << ",";
        // Number of weeks
        results_file << elements[2] << ",";
        // Min handling
        results_file << elements[3] << ",";
        // Max handling
        results_file << elements[4] << ",";
        // Num of speeds
        results_file << elements[5] << ",";
        // Bunker price
        results_file << elements[6] << ",";
        // Penalty coefficient
        results_file << elements[7] << ",";
        // Demand coefficient
        results_file << elements[8] << ",";
        // Min time window
        results_file << elements[9] << ",";
        // Max time window
        results_file << elements[10] << ",";
        // Min transit
        results_file << elements[11] << ",";
        // Max transit
        results_file << elements[12] << ",";

        results_file << std::boolalpha << prob->params.enable_cost_prize_acceleration << ",";
        results_file << std::boolalpha << prob->params.enable_prize_acceleration << ",";
        results_file << prob->params.greedy_max_outarcs << ",";
        results_file << prob->params.greedy_reruns << ",";
        results_file << prob->params.elementary_labelling_every_n_nodes << ",";
        results_file << prob->params.max_cols_to_solve_mp << ",";
        results_file << prob->params.cp_acceleration_arcs_share << ",";
        results_file << prob->params.p_acceleration_max_arcs_share << ",";

        results_file << elapsed_time << ",";
        results_file << total_time_on_master << ",";
        results_file << total_time_on_pricing << ",";
        results_file << std::setprecision(12) << ub << ",";
        results_file << std::setprecision(12) << lb << ",";
        results_file << std::setprecision(12) << gap << ",";
        results_file << std::setprecision(12) << gap_at_root << ",";
        results_file << bb_nodes_generated << ",";
        results_file << max_depth << ",";
        results_file << pool->size() << ",";

        // 1) Ships used
        auto vc_used = std::unordered_map<std::shared_ptr<VesselClass>, unsigned int>();
        auto actual_base = std::vector<Column>();

        for(const auto &vc : prob->data.vessel_classes) {
            vc_used[vc] = 0u;
        }

        if(node_bound_type == BoundType::FROM_LP) {
            for(const auto &col : node_attaining_ub->base_columns) {
                actual_base.push_back(col.first);
                vc_used[col.first.sol.vessel_class]++;
            }
        } else {
            for(const auto &col : node_attaining_ub->mip_base_columns) {
                actual_base.push_back(col.first);
                vc_used[col.first.sol.vessel_class]++;
            }
        }

        for(const auto &vc : prob->data.vessel_classes) {
            results_file << vc->name << "=" << vc_used[vc] << "/" << vc->num_vessels << " ";
        }

        results_file << ",";

        auto total_used = std::accumulate(
            vc_used.begin(),
            vc_used.end(),
            0u,
            [&](unsigned int cum, const auto &vc_n) {
                return cum + vc_n.second;
            }
        );

        auto total_vessels = std::accumulate(
            prob->data.vessel_classes.begin(),
            prob->data.vessel_classes.end(),
            0u,
            [&](unsigned int cum, const auto &vc) {
                return cum + vc->num_vessels;
            }
        );

        results_file << total_used << "," << total_vessels << ",";

        // 2.a) Average length of a rotation
        auto rot_lengths = 0.0;

        for(const auto &col : actual_base) {
            rot_lengths += col.sol.length();
        }

        results_file << rot_lengths / actual_base.size() << ",";

        // 2.b) Average number of ports visited
        auto n_ports_visited = 0.0;

        for(const auto& col : actual_base) {
            n_ports_visited += col.sol.n_ports_visited();
        }

        results_file << (n_ports_visited / actual_base.size()) << ",";

        // 2.c) Average highest load efficiency
        auto tot_highest_load_efficiency = 0.0;

        for(const auto& col : actual_base) {
            tot_highest_load_efficiency += col.sol.highest_load_efficiency();
        }

        results_file << (tot_highest_load_efficiency / actual_base.size()) << ",";

        // 3) Average travel distance of cargo
        auto distance_sum = 0.0;
        auto distance_n = 0u;

        for(const auto &col : actual_base) {
            auto d = col.sol.cargo_travel_distances();
            distance_sum += std::accumulate(d.begin(), d.end(), 0.0);
            distance_n += d.size();
        }

        results_file << (distance_sum / distance_n) << ",";

        // 4) Average speed
        auto weighted_speed = 0.0;
        auto total_distance = 0.0;

        for(const auto &col : actual_base) {
            auto d = col.sol.legs_distance();
            auto s = col.sol.legs_speed();

            assert(d.size() == s.size());

            for(auto i = 0u; i < d.size(); i++) {
                // Exclude delivery-to-pickup arcs
                if(d[i] > 0.0001) {
                    weighted_speed += d[i] * s[i];
                    total_distance += d[i];
                }
            }
        }

        results_file << (weighted_speed / total_distance) << ",";

        // 5) Served cargoes
        auto served_cargoes = 0u;

        for(const auto &col : actual_base) {
            served_cargoes += col.sol.path.size() - 1;
        }

        results_file << served_cargoes << ",";

        // 6) Bunker Cost Analysis
        SolutionCosts global;
        for(const auto &col : actual_base) {
            global.add(col.sol.solution_costs());
        }

        results_file << global.total_bunker / global.total_costs;

        results_file << std::endl;
        results_file.close();
    }

    void BBTree::print_row(const BBNode &current_node, double gap_node) const {
        auto print_ub = (ub < std::numeric_limits<double>::max() - 100);

        std::cout << std::fixed;
        std::cout << std::setw(10) << unexplored_nodes.size() << "  ";
        std::cout << std::setw(8) << bb_nodes_generated << "  ";
        std::cout << std::setw(12) << std::setprecision(2) << current_node.sol_value << "  ";
        std::cout << std::setw(12) << std::setprecision(2) << lb << "  ";
        if(print_ub) {
            std::cout << std::setw(14) << std::setprecision(2) << ub << "  ";
        } else {
            std::cout << std::setw(14) << "inf" << "  ";
        }
        std::cout << std::setw(11) << std::setprecision(4) << gap_node << "\%  ";
        std::cout << std::setw(11) << std::setprecision(4) << gap << "\%  ";
        std::cout << std::setw(8) << pool->size() << "  ";
        std::cout << std::setw(20) << std::setprecision(4) << current_node.total_time_spent_on_mp << "  ";
        std::cout << std::setw(20) << std::setprecision(4) << current_node.total_time_spent_on_sp << "  ";
        std::cout << std::setw(20) << std::setprecision(4) << current_node.total_time_spent << "  ";
        std::cout << std::setw(20) << std::setprecision(4) << current_node.avg_time_spent_on_sp << "  ";
        std::cout << std::setw(24) << std::setprecision(4) << current_node.max_time_spent_by_exact_solver << "  ";
        std::cout << std::setw(6) << current_node.depth << "  " << std::endl;
        defaultfloat(std::cout);
    }

    void BBTree::print_summary() const {
        auto all_penalties = 0.0;
        for(const auto &p : prob->data.ports) {
            all_penalties += p->pickup_penalty + p->delivery_penalty;
        }

        std::cout << std::endl << "*** SOLUTION ***" << std::endl;
        std::cout << "Total cost: " << ub << " (all penalties: " << all_penalties << ")" << std::endl;

        if(node_bound_type == BoundType::FROM_LP) {
            // UB was attained as LP solution
            std::cout << "*** OBTAINED FROM LP ***" << std::endl;
            for(const auto &cc : node_attaining_ub->base_columns) {
                std::cout << cc.first << " selected with coefficient " << cc.second << std::endl;
                cc.first.sol.g->print_path(cc.first.sol.path, std::cout);
            }
        } else {
            // UB was attained as MIP solution
            std::cout << "*** OBTAINED FROM MIP ***" << std::endl;
            for(const auto &cc : node_attaining_ub->mip_base_columns) {
                std::cout << cc.first << " selected with coefficient " << cc.second << std::endl;
                cc.first.sol.g->print_path(cc.first.sol.path, std::cout);
            }
        }
    }
}
