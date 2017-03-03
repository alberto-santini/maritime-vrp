//
// Created by alberto on 27/02/17.
//

#include "branching_rule.h"

namespace mvrp {
    /* --- Include Port --- */
    void IncludePort::add_erased_edges(const Graph& graph, ErasedEdges& erased) const {}
    bool IncludePort::is_column_compatible(const Column& column) const { return true; }
    bool IncludePort::should_row_be_equality(const Port& port, const PortType& pu_type) const {
        return port == *(this->port) && pu_type == this->pu_type;
    }

    /* --- Exclude Port --- */
    void ExcludePort::add_erased_edges(const Graph& graph, ErasedEdges& erased) const {
        for(auto eit = boost::edges(graph.graph); eit.first != eit.second; ++eit.first) {
            auto v_source = boost::source(*eit.first, graph.graph);
            auto v_target = boost::target(*eit.first, graph.graph);
            const auto& n_source = graph.graph[v_source];
            const auto& n_target = graph.graph[v_target];

            if(
                (*n_source->port == *port && n_source->pu_type == pu_type) ||
                (*n_target->port == *port && n_target->pu_type == pu_type)
            ) {
                if(erased.find(v_source) == erased.end()) { erased[v_source] = std::set<Edge>(); }
                erased[v_source].insert(*eit.first);
            }
        }
    }
    bool ExcludePort::is_column_compatible(const Column& column) const {
        if(column.dummy) { return true; }
        return !column.sol.visits_port(*port, pu_type);
    }
    bool ExcludePort::should_row_be_equality(const Port& port, const PortType& pu_type) const { return false; }

    /* --- Assign Port to Vessel --- */
    void AssignToVessel::add_erased_edges(const Graph& graph, ErasedEdges& erased) const {
        if(*(graph.vessel_class) == *vc) { return; }

        for(auto eit = boost::edges(graph.graph); eit.first != eit.second; ++eit.first) {
            auto v_source = boost::source(*eit.first, graph.graph);
            auto v_target = boost::target(*eit.first, graph.graph);
            const auto& n_source = graph.graph[v_source];
            const auto& n_target = graph.graph[v_target];

            if(
                (*n_source->port == *port && n_source->pu_type == pu_type) ||
                (*n_target->port == *port && n_target->pu_type == pu_type)
                ) {
                if(erased.find(v_source) == erased.end()) { erased[v_source] = std::set<Edge>(); }
                erased[v_source].insert(*eit.first);
            }
        }
    }
    bool AssignToVessel::is_column_compatible(const Column& column) const {
        if(column.dummy) { return true; }
        if(*(column.sol.vessel_class) == *vc) { return true; }
        return !column.sol.visits_port(*port, pu_type);
    }
    bool AssignToVessel::should_row_be_equality(const Port& port, const PortType& pu_type) const { return false; }

    /* --- Forbid Port to Vessel --- */
    void ForbidToVessel::add_erased_edges(const Graph& graph, ErasedEdges& erased) const {
        if(*(graph.vessel_class) != *vc) { return; }

        for(auto eit = boost::edges(graph.graph); eit.first != eit.second; ++eit.first) {
            auto v_source = boost::source(*eit.first, graph.graph);
            auto v_target = boost::target(*eit.first, graph.graph);
            const auto& n_source = graph.graph[v_source];
            const auto& n_target = graph.graph[v_target];

            if(
                (*n_source->port == *port && n_source->pu_type == pu_type) ||
                (*n_target->port == *port && n_target->pu_type == pu_type)
            ) {
                if(erased.find(v_source) == erased.end()) { erased[v_source] = std::set<Edge>(); }
                erased[v_source].insert(*eit.first);
            }
        }
    }
    bool ForbidToVessel::is_column_compatible(const Column& column) const {
        if(column.dummy) { return true; }
        if(*(column.sol.vessel_class) != *vc) { return true; }
        return !column.sol.visits_port(*port, pu_type);
    }
    bool ForbidToVessel::should_row_be_equality(const Port& port, const PortType& pu_type) const { return false; }

    /* --- Force Consecutive Visit of Two Ports --- */
    void ForceConsecutiveVisit::add_erased_edges(const Graph& graph, ErasedEdges& erased) const {
        if(*(graph.vessel_class) != *vc) { return; }

        const auto& f_src = consec.first;
        const auto& f_trg = consec.second;

        for(auto eit = boost::edges(graph.graph); eit.first != eit.second; ++eit.first) {
            auto v_source = boost::source(*eit.first, graph.graph);
            auto v_target = boost::target(*eit.first, graph.graph);
            const auto& n_source = graph.graph[v_source];
            const auto& n_target = graph.graph[v_target];

            if( (*n_source->port == *f_src.first && n_source->pu_type == f_src.second &&
                (*n_target->port != *f_trg.first || n_target->pu_type != f_trg.second)) ||
                (*n_target->port == *f_trg.first && n_target->pu_type == f_src.second &&
                (*n_source->port != *f_src.first || n_source->pu_type != f_src.second))
            ) {
                if(erased.find(v_source) == erased.end()) { erased[v_source] = std::set<Edge>(); }
                erased[v_source].insert(*eit.first);
            }
        }
    }
    bool ForceConsecutiveVisit::is_column_compatible(const Column& column) const {
        if(column.dummy) { return true; }
        if(*(column.sol.vessel_class) != *vc) { return true; }
        return column.sol.visits_consecutive_ports(consec.first, consec.second);
    }
    bool ForceConsecutiveVisit::should_row_be_equality(const Port& port, const PortType& pu_type) const { return false; }

    /* --- Forbid Consecutive Visit of Two Ports --- */
    void ForbidConsecutiveVisit::add_erased_edges(const Graph& graph, ErasedEdges& erased) const {
        if(*(graph.vessel_class) != *vc) { return; }

        const auto& f_src = consec.first;
        const auto& f_trg = consec.second;

        for(auto eit = boost::edges(graph.graph); eit.first != eit.second; ++eit.first) {
            auto v_source = boost::source(*eit.first, graph.graph);
            auto v_target = boost::target(*eit.first, graph.graph);
            const auto& n_source = graph.graph[v_source];
            const auto& n_target = graph.graph[v_target];

            if( *n_source->port == *f_src.first && n_source->pu_type == f_src.second &&
                *n_target->port == *f_trg.first && n_target->pu_type == f_trg.second
            ) {
                if(erased.find(v_source) == erased.end()) { erased[v_source] = std::set<Edge>(); }
                erased[v_source].insert(*eit.first);
            }
        }
    }
    bool ForbidConsecutiveVisit::is_column_compatible(const Column& column) const {
        if(column.dummy) { return true; }
        if(*(column.sol.vessel_class) != *vc) { return true; }
        return !column.sol.visits_consecutive_ports(consec.first, consec.second);
    }
    bool ForbidConsecutiveVisit::should_row_be_equality(const Port& port, const PortType& pu_type) const { return false; }

    /* --- Force Speed --- */
    void ForceSpeed::add_erased_edges(const Graph& graph, ErasedEdges& erased) const {
        if(*(graph.vessel_class) != *vc) { return; }

        const auto& f_src = std::get<0>(cons_spd);
        const auto& f_trg = std::get<1>(cons_spd);

        for(auto eit = boost::edges(graph.graph); eit.first != eit.second; ++eit.first) {
            auto v_source = boost::source(*eit.first, graph.graph);
            auto v_target = boost::target(*eit.first, graph.graph);
            const auto& n_source = graph.graph[v_source];
            const auto& n_target = graph.graph[v_target];

            if( *n_source->port == *f_src.first && n_source->pu_type == f_src.second &&
                *n_target->port == *f_trg.first && n_target->pu_type == f_trg.second &&
                std::abs(graph.graph[*eit.first]->speed - std::get<2>(cons_spd)) > 1e-3
            ) {
                if(erased.find(v_source) == erased.end()) { erased[v_source] = std::set<Edge>(); }
                erased[v_source].insert(*eit.first);
            }
        }
    }
    bool ForceSpeed::is_column_compatible(const Column& column) const {
        if(column.dummy) { return true; }
        if(*(column.sol.vessel_class) != *vc) { return true; }
        return column.sol.visits_consecutive_ports_at_speed(std::get<0>(cons_spd), std::get<1>(cons_spd), std::get<2>(cons_spd));
    }
    bool ForceSpeed::should_row_be_equality(const Port& port, const PortType& pu_type) const { return false; }

    /* --- Forbid Speed --- */
    void ForbidSpeed::add_erased_edges(const Graph& graph, ErasedEdges& erased) const {
        if(*(graph.vessel_class) != *vc) { return; }

        const auto& f_src = std::get<0>(cons_spd);
        const auto& f_trg = std::get<1>(cons_spd);

        for(auto eit = boost::edges(graph.graph); eit.first != eit.second; ++eit.first) {
            auto v_source = boost::source(*eit.first, graph.graph);
            auto v_target = boost::target(*eit.first, graph.graph);
            const auto& n_source = graph.graph[v_source];
            const auto& n_target = graph.graph[v_target];

            if( *n_source->port == *f_src.first && n_source->pu_type == f_src.second &&
                *n_target->port == *f_trg.first && n_target->pu_type == f_trg.second &&
                std::abs(graph.graph[*eit.first]->speed - std::get<2>(cons_spd)) < 1e-3
                ) {
                if(erased.find(v_source) == erased.end()) { erased[v_source] = std::set<Edge>(); }
                erased[v_source].insert(*eit.first);
            }
        }
    }
    bool ForbidSpeed::is_column_compatible(const Column& column) const {
        if(column.dummy) { return true; }
        if(*(column.sol.vessel_class) != *vc) { return true; }
        return !column.sol.visits_consecutive_ports_at_speed(std::get<0>(cons_spd), std::get<1>(cons_spd), std::get<2>(cons_spd));
    }
    bool ForbidSpeed::should_row_be_equality(const Port& port, const PortType& pu_type) const { return false; }

    /* --- Force Arc --- */
    void ForceArc::add_erased_edges(const Graph& graph, ErasedEdges& erased) const {
        if(*(graph.vessel_class) != *vc) { return; }

        const auto& f_src = *graph.graph[boost::source(e, graph.graph)];
        const auto& f_trg = *graph.graph[boost::target(e, graph.graph)];

        for(auto eit = boost::edges(graph.graph); eit.first != eit.second; ++eit.first) {
            auto v_source = boost::source(*eit.first, graph.graph);
            auto v_target = boost::target(*eit.first, graph.graph);
            const auto& n_source = graph.graph[v_source];
            const auto& n_target = graph.graph[v_target];

            if(n_source->same_row_as(f_src) && n_target->same_row_as(f_trg) && *eit.first != e) {
                if(erased.find(v_source) == erased.end()) { erased[v_source] = std::set<Edge>(); }
                erased[v_source].insert(*eit.first);
            }
        }
    }
    bool ForceArc::is_column_compatible(const Column& column) const {
        if(column.dummy) { return true; }
        if(*(column.sol.vessel_class) != *vc) { return true; }
        return column.sol.uses_arc(e);
    }
    bool ForceArc::should_row_be_equality(const Port& port, const PortType& pu_type) const { return false; }

    /* --- Forbid Arc --- */
    void ForbidArc::add_erased_edges(const Graph& graph, ErasedEdges& erased) const {
        auto v_source = boost::source(e, graph.graph);
        if(erased.find(v_source) == erased.end()) { erased[v_source] = std::set<Edge>(); }
        erased[v_source].insert(e);
    }
    bool ForbidArc::is_column_compatible(const Column& column) const {
        if(column.dummy) { return true; }
        if(*(column.sol.vessel_class) != *vc) { return true; }
        return !column.sol.uses_arc(e);
    }
    bool ForbidArc::should_row_be_equality(const Port& port, const PortType& pu_type) const { return false; }
}