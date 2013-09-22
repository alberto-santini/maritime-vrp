#include <branching/bb_node.h>

BBNode::BBNode(const Problem& prob, const Problem local_prob, ColumnPool& pool, const ColumnPool local_pool, const VisitRuleList unite_rules, const VisitRuleList separate_rules, const PortDuals port_duals, const VcDuals vc_duals) : prob(prob), local_prob(local_prob), pool(pool), local_pool(local_pool), unite_rules(unite_rules), separate_rules(separate_rules), port_duals(port_duals), vc_duals(vc_duals) {
    make_local_graphs();
}

void BBNode::populate_pool() {
    remove_incompatible_columns();
    generate_nrc_columns();
}

void BBNode::make_local_graphs() {
    for(const VisitRule& vr : unite_rules) {
        std::shared_ptr<VesselClass> vc = vr.first->vessel_class;
        Graph& g = local_prob.graphs.at(vc);
        g.unite_ports(vr, g);
    }
    for(const VisitRule& vr : separate_rules) {
        std::shared_ptr<VesselClass> vc = vr.first->vessel_class;
        Graph& g = local_prob.graphs.at(vc);
        g.separate_ports(vr, g);
    }
    for(std::shared_ptr<VesselClass> vc : local_prob.data.vessel_classes) {
        Graph& g = local_prob.graphs.at(vc);
        g.port_duals = port_duals;
        g.vc_dual = vc_duals.at(vc);
    }
}

void BBNode::remove_incompatible_columns() {
    ColumnPool new_local_pool;
    
    for(const Column& c : local_pool) {
        bool compatible = true;
        
        for(const VisitRule& vr : unite_rules) {
            if(!c.is_compatible_with_unite_rule(vr)) {
                compatible = false;
                break;
            }
        }
        
        if(!compatible) {
            break;
        }
        
        for(const VisitRule& vr : separate_rules) {
            if(!c.is_compatible_with_separate_rule(vr)) {
                compatible = false;
                break;
            }
        }
        
        if(!compatible) {
            break;
        }
        
        new_local_pool.push_back(c.transfer_to(local_prob));
    }
    
    local_pool = new_local_pool;
}

void BBNode::generate_nrc_columns() {
    ColumnPool nrc_columns = local_pool;
    SPSolver solv(local_prob);
    solv.solve(nrc_columns);
    
    for(const Column c : nrc_columns) {
        local_pool.push_back(c);
        pool.push_back(c.transfer_to(prob));
    }
}