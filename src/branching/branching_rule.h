//
// Created by alberto on 27/02/17.
//

#ifndef MARITIME_VRP_BRANCHING_RULE_H
#define MARITIME_VRP_BRANCHING_RULE_H

#include "../base/graph.h"
#include "../column/column.h"

namespace mvrp {
    class BranchingRule {
    public:
        virtual void add_erased_edges(const Graph& graph, ErasedEdges& erased) const = 0;
        virtual bool is_column_compatible(const Column& column) const = 0;
        virtual bool should_row_be_equality(const Port& port, const PortType& pu_type) const = 0;
    };

    class IncludePort : public BranchingRule {
        const Port* port;
        const PortType pu_type;

    public:
        IncludePort(const Port* port, PortType pu_type) : port{port}, pu_type{pu_type} {}

        void add_erased_edges(const Graph& graph, ErasedEdges& erased) const;
        bool is_column_compatible(const Column& column) const;
        bool should_row_be_equality(const Port& port, const PortType& pu_type) const;
    };

    class ExcludePort : public BranchingRule {
        const Port* port;
        const PortType pu_type;

    public:
        ExcludePort(const Port* port, PortType pu_type) : port{port}, pu_type{pu_type} {}

        void add_erased_edges(const Graph& graph, ErasedEdges& erased) const;
        bool is_column_compatible(const Column& column) const;
        bool should_row_be_equality(const Port& port, const PortType& pu_type) const;
    };

    class AssignToVessel : public BranchingRule {
        const Port* port;
        const PortType pu_type;
        const VesselClass* vc;

    public:
        AssignToVessel(const Port* port, PortType pu_type, const VesselClass* vc) : port{port}, pu_type{pu_type}, vc{vc} {}

        void add_erased_edges(const Graph& graph, ErasedEdges& erased) const;
        bool is_column_compatible(const Column& column) const;
        bool should_row_be_equality(const Port& port, const PortType& pu_type) const;
    };

    class ForbidToVessel : public BranchingRule {
        const Port* port;
        const PortType pu_type;
        const VesselClass* vc;

    public:
        ForbidToVessel(const Port* port, PortType pu_type, const VesselClass* vc) : port{port}, pu_type{pu_type}, vc{vc} {}

        void add_erased_edges(const Graph& graph, ErasedEdges& erased) const;
        bool is_column_compatible(const Column& column) const;
        bool should_row_be_equality(const Port& port, const PortType& pu_type) const;
    };

    class ForceConsecutiveVisit : public BranchingRule {
        const std::pair<PortWithType, PortWithType> consec;
        const VesselClass* vc;

    public:
        ForceConsecutiveVisit(std::pair<PortWithType, PortWithType> consec, VesselClass* vc) : consec{consec}, vc{vc} {}

        void add_erased_edges(const Graph& graph, ErasedEdges& erased) const;
        bool is_column_compatible(const Column& column) const;
        bool should_row_be_equality(const Port& port, const PortType& pu_type) const;
    };

    class ForbidConsecutiveVisit : public BranchingRule {
        const std::pair<PortWithType, PortWithType> consec;
        const VesselClass* vc;

    public:
        ForbidConsecutiveVisit(std::pair<PortWithType, PortWithType> consec, VesselClass* vc) : consec{consec}, vc{vc} {}

        void add_erased_edges(const Graph& graph, ErasedEdges& erased) const;
        bool is_column_compatible(const Column& column) const;
        bool should_row_be_equality(const Port& port, const PortType& pu_type) const;
    };

    class ForceSpeed : public BranchingRule {
        const std::tuple<PortWithType, PortWithType, double> cons_spd;
        const VesselClass* vc;

    public:
        ForceSpeed(std::tuple<PortWithType, PortWithType, double> cons_spd, VesselClass* vc) : cons_spd{cons_spd}, vc{vc} {}

        void add_erased_edges(const Graph& graph, ErasedEdges& erased) const;
        bool is_column_compatible(const Column& column) const;
        bool should_row_be_equality(const Port& port, const PortType& pu_type) const;
    };

    class ForbidSpeed : public BranchingRule {
        const std::tuple<PortWithType, PortWithType, double> cons_spd;
        const VesselClass* vc;

    public:
        ForbidSpeed(std::tuple<PortWithType, PortWithType, double> cons_spd, VesselClass* vc) : cons_spd{cons_spd}, vc{vc} {}

        void add_erased_edges(const Graph& graph, ErasedEdges& erased) const;
        bool is_column_compatible(const Column& column) const;
        bool should_row_be_equality(const Port& port, const PortType& pu_type) const;
    };

    class ForceArc : public BranchingRule {
        const Edge e;
        const VesselClass* vc;

    public:
        ForceArc(Edge e, VesselClass* vc) : e{e}, vc{vc} {}

        void add_erased_edges(const Graph& graph, ErasedEdges& erased) const;
        bool is_column_compatible(const Column& column) const;
        bool should_row_be_equality(const Port& port, const PortType& pu_type) const;
    };

    class ForbidArc : public BranchingRule {
        const Edge e;
        const VesselClass* vc;

    public:
        ForbidArc(Edge e, VesselClass* vc) : e{e}, vc{vc} {}

        void add_erased_edges(const Graph& graph, ErasedEdges& erased) const;
        bool is_column_compatible(const Column& column) const;
        bool should_row_be_equality(const Port& port, const PortType& pu_type) const;
    };
}

#endif //MARITIME_VRP_BRANCHING_RULE_H
