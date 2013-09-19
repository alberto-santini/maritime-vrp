#include <branching/node.h>

BBNode::BBNode(const Problem prob, ColumnPool& pool, const VisitRuleList unite_rules, const VisitRuleList separate_rules) : prob(prob), pool(pool), unite_rules(unite_rules), separate_rules(separate_rules) {
    make_local_graphs();
}

void BBNode::populate_pool() {
    copy_compatible_columns();
    generate_nrc_columns();
}

void BBNode::make_local_graphs() {
    for(VisitRule vr : unite_rules) {
        std::shared_ptr<VesselClass> vc = vr.first->vessel_class;
        Graph& g = prob.graphs.at(vc);
        g.unite_ports(vr, g);
    }
    for(VisitRule vr : separate_rules) {
        std::shared_ptr<VesselClass> vc = vr.first->vessel_class;
        Graph& g = prob.graphs.at(vc);
        g.separate_ports(vr, g);
    }
}

void BBNode::copy_compatible_columns() {
    for(const Column& c : pool) {
        bool compatible = true;
        
        for(VisitRule vr : unite_rules) {
            if(!c.is_compatible_with_unite_rule(vr)) {
                compatible = false;
                break;
            }
        }
        
        if(!compatible) {
            break;
        }
        
        for(VisitRule vr : separate_rules) {
            if(!c.is_compatible_with_separate_rule(vr)) {
                compatible = false;
                break;
            }
        }
        
        if(!compatible) {
            break;
        }
        
        local_pool.push_back(c);
    }
}

void BBNode::generate_nrc_columns() {
    // Todo...
}