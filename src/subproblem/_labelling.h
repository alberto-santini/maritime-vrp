//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef LABELLING_H
#define LABELLING_H

#include <set>
#include <map>
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <boost/optional.hpp>

#include <base/graph.h>
#include <column/solution.h>

using VisitablePorts = std::vector<std::pair<std::shared_ptr<Port>, PickupType>>;

class Label {
public:
    const Graph& g;
    int pic;
    int del;
    double cost;
    
    static constexpr double EPS = 0.001;
    
    Label(  const Graph& g,
            int pic,
            int del,
            double cost) :
            g{g},
            pic{pic},
            del{del},
            cost{cost} {}
            
    Label(  const Graph& g) :
            g{g},
            pic{g.vessel_class->capacity},
            del{g.vessel_class->capacity},
            cost{0.0} {}
};

class ElementaryLabel : public Label {
public:
    VisitablePorts por;
    
    ElementaryLabel(const Graph& g,
                    int pic,
                    int del,
                    double cost,
                    VisitablePorts por) : 
                    Label{g, pic, del, cost},
                    por{por} {}
                    
    ElementaryLabel(const Graph& g,
                    VisitablePorts por) :
                    Label{g},
                    por{por} {}
                    
};

class LabelExtender {
public:
    const ErasedEdges& erased;
    
    LabelExtender(const ErasedEdges& erased) : erased{erased} {}

    boost::optional<Label> operator()(const BGraph& graph, const Label& label, const Edge& e) const;
    boost::optional<ElementaryLabel> operator()(const BGraph& graph, const ElementaryLabel& label, const Edge& e) const;
};

/* ============================================= */

template<typename Lbl>
class LblContainer {
public:
    uint32_t id;
    std::vector<uint32_t> predecessors;
        
    Lbl label;
    const LblContainer* pred_container;
    const BGraph& g;
    boost::optional<const Edge> pred_edge;
    
    LblContainer(   uint32_t id,
                    std::vector<uint32_t> predecessors,
                    Lbl label,
                    const LblContainer* pred_container,
                    const BGraph& g,
                    const Edge pred_edge) :
                    id{id},
                    predecessors{predecessors},
                    label{label},
                    pred_container{pred_container},
                    g{g},
                    pred_edge{pred_edge} {}
    
    LblContainer(   Lbl label,
                    const BGraph& g) :
                    id{1u},
                    predecessors{std::vector<uint32_t>()},
                    label{label},
                    pred_container{nullptr},
                    g{g},
                    pred_edge{boost::none} {}
};

template<typename Lbl>
struct LblContainerComp {
  bool operator()(const LblContainer<Lbl>& c1, const LblContainer<Lbl>& c2) const {
      return c1.label.cost < c2.label.cost;
  }  
};

template<typename Lbl>
using ContainersSet = std::set<LblContainer<Lbl>, LblContainerComp<Lbl>>;

template<typename Lbl>
using VertexContainersMap = std::map<Vertex, ContainersSet<Lbl>>;

template<typename Lbl>
bool operator==(const LblContainer<Lbl>& c1, const LblContainer<Lbl>& c2) {
    if(c1.pred_edge != c2.pred_edge) { return false; }
    if(c1.pred_container != c2.pred_container) { return false; }
    return c1.label == c2.label;
}

template<typename Lbl>
struct OfflineContainer {
    uint32_t id;
    uint32_t fid;
    uint32_t domby;
    bool dom;
    Vertex v;
    Edge e;
    Lbl l;

    OfflineContainer(Vertex v, Lbl l) : id{1}, fid{0}, domby{0}, dom{false}, v{v}, e{Edge()} , l{l} {}
    OfflineContainer(uint32_t id, uint32_t fid, uint32_t domby, bool dom, Vertex v, Edge e, Lbl l) : id{id}, fid{fid}, domby{domby}, dom{dom}, v{v}, e{e}, l{l} {}

    void print(const Graph& g) const {
        assert(dom || domby == 0u);
        assert(!dom || domby > 0u);

        std::cout << "\t" << *g.graph[v] << " {id: " << id << ", fid: " << fid << ", dom: ";
        if(dom) { std::cout << domby; } else { std::cout << "no"; }
        std::cout << ", l: " << l << "}" << std::endl;
    }
};

template<typename Lbl, typename LblExt>
class LabellingAlgorithm {
    std::shared_ptr<const Graph> g;
    
    void diagnose_invalid(const std::vector<OfflineContainer<Lbl>>& c, const std::vector<uint32_t>& invalid, const LblContainer<Lbl>& container) const;
    void set_dominated(std::vector<OfflineContainer<Lbl>>& c, uint32_t id, uint32_t domid) const;
public:
    LabellingAlgorithm(std::shared_ptr<const Graph> g) : g{g} {}
    
    std::vector<Solution> solve(Vertex start_v, Vertex end_v, Lbl start_label, LblExt extension) const;
};

bool operator==(const Label& lhs, const Label& rhs);
bool operator!=(const Label& lhs, const Label& rhs);
bool operator<=(const Label& lhs, const Label& rhs);
bool operator<(const Label& lhs, const Label& rhs);
std::ostream& operator<<(std::ostream& out, const Label& l);

bool operator==(const ElementaryLabel& lhs, const ElementaryLabel& rhs);
bool operator!=(const ElementaryLabel& lhs, const ElementaryLabel& rhs);
bool operator<=(const ElementaryLabel& lhs, const ElementaryLabel& rhs);
bool operator<(const ElementaryLabel& lhs, const ElementaryLabel& rhs);
std::ostream& operator<<(std::ostream& out, const ElementaryLabel& l);

template<typename Lbl>
void print_map(const VertexContainersMap<Lbl>& m, const Graph& g) {
    for(const auto& vertex_set : m) {
        const Vertex& v = vertex_set.first;
        const ContainersSet<Lbl>& s = vertex_set.second;
        std::cout << *g.graph[v] << " => " << std::endl;
        for(const auto& container : s) { std::cout << "\t" << container.label << std::endl; }
    }
}

template<typename Lbl, typename LblExt>
void LabellingAlgorithm<Lbl, LblExt>::set_dominated(std::vector<OfflineContainer<Lbl>>& c, uint32_t id, uint32_t domid) const {
    for(auto& oc : c) { if(oc.id == id) { oc.dom = true; oc.domby = domid; return; } }
}

template<typename Lbl, typename LblExt>
void LabellingAlgorithm<Lbl, LblExt>::diagnose_invalid(const std::vector<OfflineContainer<Lbl>>& c, const std::vector<uint32_t>& invalid, const LblContainer<Lbl>& container) const {
    std::cout << "=======================================" << std::endl;

    /*******************************************************/
    std::cout << "Partial path:" << std::endl;
    for(auto label_id : container.predecessors) {
        auto it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == label_id; });
        assert(it != c.end());
        it->print(*g);
    }
    auto container_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == container.id; });
    container_it->print(*g);
    std::cout << std::endl;

    /*******************************************************/
    auto path_it = std::find_if(container.predecessors.begin(), container.predecessors.end(),
        [&] (const auto& p) { return std::find(invalid.begin(), invalid.end(), p) != invalid.end(); });

    auto invalid_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == *path_it; });

    assert(invalid_it != c.end());
    assert(invalid_it->dom);

    auto dominant_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == invalid_it->domby; });

    assert(dominant_it != c.end());

    std::cout << "=== Invalid" << std::endl;
    invalid_it->print(*g);
    std::cout << "=== Dominant" << std::endl;
    dominant_it->print(*g);
    std::cout << std::endl;

    assert(invalid_it->v == dominant_it->v);

    /*******************************************************/
    while(true) {
        uint32_t cur_id = 0;

        if(cur_id == container.id) { break; }

        if(++path_it == container.predecessors.end()) {
            cur_id = container.id;
        } else {
            cur_id = *path_it;
        }

        auto prev_invalid_it = invalid_it;
        auto prev_dominant_it = dominant_it;

        invalid_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == cur_id; });

        assert(invalid_it->fid == prev_invalid_it->id);

        dominant_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.fid == dominant_it->id && oc.v == invalid_it->v; });

        assert(invalid_it->v == dominant_it->v);

        std::cout << "=== [Prev] Invalid" << std::endl;
        prev_invalid_it->print(*g);
        std::cout << "=== [New] Invalid" << std::endl;
        invalid_it->print(*g);

        if(invalid_it->dom) {
            auto new_dom_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == invalid_it->domby; });
            std::cout << "=== [New] Invalid dominated by" << std::endl;
            new_dom_it->print(*g);
        }

        std::cout << "=== [Prev] Dominant" << std::endl;
        prev_dominant_it->print(*g);
        std::cout << "=== [New] Dominant" << std::endl;
        dominant_it->print(*g);

        if(!invalid_it->dom) {
            std::cout << "[New] Invalid not dominated. Quitting." << std::endl;
            break;
        }

        std::cout << std::endl;
    }
}

template<typename Lbl, typename LblExt>
std::vector<Solution> LabellingAlgorithm<Lbl, LblExt>::solve(Vertex start_v, Vertex end_v, Lbl start_label, LblExt extension) const {
    VertexContainersMap<Lbl> undominated;
    VertexContainersMap<Lbl> unprocessed;
    std::vector<uint32_t> invalid;
    /* DBG */ std::vector<OfflineContainer<Lbl>> c;
    
    // In the beginning we only have the starting label, as an unprocessed label at the starting vertex
    unprocessed[start_v] = { LblContainer<Lbl>(start_label, g->graph) };
    
    /* DBG */ c.emplace_back(start_v, start_label);
    
    uint32_t container_id = 2u;
        
    // While there are unprocessed labels...
    while(!unprocessed.empty()) {
        // Get the first vertex with unprocessed labels
        auto any_set_it = unprocessed.begin();
        Vertex cur_vertex = any_set_it->first;
        const ContainersSet<Lbl>& containers_at_cur_vertex = any_set_it->second;
        
        // Get the first unprocessed label at the selected vertex
        assert(!containers_at_cur_vertex.empty());
        auto any_cnt_it = containers_at_cur_vertex.begin();
        
        // Make a copy of the chosen label
        auto cur_container = LblContainer<Lbl>(*any_cnt_it);
        bool cur_valid = true;
        
        if( cur_container.pred_container &&
                (!cur_container.pred_edge ||
                std::any_of(cur_container.predecessors.begin(), cur_container.predecessors.end(),
                    [&] (const auto& i) { return std::find(invalid.begin(), invalid.end(), i) != invalid.end(); }))
        ) { cur_valid = false; }
        
        typename ContainersSet<Lbl>::iterator cur_inserted_it;
        bool cur_inserted;
        
        // If the current label is valid, insert it in undominated
        if(cur_valid) {
            if(undominated.find(cur_vertex) == undominated.end()) { undominated[cur_vertex] = ContainersSet<Lbl>(); }
            std::tie(cur_inserted_it, cur_inserted) = undominated.at(cur_vertex).insert(cur_container);
            assert(!cur_inserted_it->pred_container || cur_inserted_it->pred_edge);
        }
        else {
            diagnose_invalid(c, invalid, cur_container);
        }
        
        // In any case, remove the label from unprocessed
        unprocessed.at(cur_vertex).erase(any_cnt_it);
        if(unprocessed.at(cur_vertex).empty()) { unprocessed.erase(cur_vertex); }
        
        // If the label was not valid, mode to the next one
        if(!cur_valid) { continue; }
                
        // Try to expand the current label along all out-edges departing from the current vertex
        for(auto oe = out_edges(cur_vertex, g->graph); oe.first != oe.second; ++oe.first) {
            Edge e = *oe.first;
            Vertex dest_vertex = target(e, g->graph);
            
            // Call to the extension function
            auto new_label = extension(g->graph, cur_container.label, e);
            
            // Extension didn't succeed: skip the rest
            if(!new_label) { continue; }
            
            // Extension succeeded! Create a container for the new label
            auto new_container = LblContainer<Lbl>(
                container_id++,
                cur_container.predecessors, 
                *new_label,
                &(*cur_inserted_it),
                g->graph,
                e);
            new_container.predecessors.push_back(cur_container.id);
            bool new_container_dominated = false;
            
            /* DBG */ c.emplace_back(container_id - 1, cur_container.id, 0, false, dest_vertex, e, *new_label);
                        
            // If there are unprocessed labels at the destination vertex,
            // if any of them dominates the new label, then discard the new label;
            // if the new label dominates any of them, then discard them.
            if(unprocessed.find(dest_vertex) != unprocessed.end()) {
                auto dest_unp_cnt_it = unprocessed.at(dest_vertex).begin();
                
                while(dest_unp_cnt_it != unprocessed.at(dest_vertex).end()) {
                    const LblContainer<Lbl>& dest_container = *dest_unp_cnt_it;
                    
                    if(new_container.label < dest_container.label && !(dest_container.label < new_container.label)) {
                        /* DBG */ {
                            auto dominated_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == dest_container.id; });
                            auto dominant_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == new_container.id; });
                            assert(dominated_it->v == dominant_it->v);
                        }
                        /* DBG */ set_dominated(c, dest_container.id, new_container.id);
                        assert(target(*new_container.pred_edge, g->graph) == target(*dest_container.pred_edge, g->graph));
                        invalid.push_back(dest_container.id);
                        unprocessed.at(dest_vertex).erase(dest_unp_cnt_it++);
                    } else if(dest_container.label < new_container.label && !(new_container.label < dest_container.label)) {
                        /* DBG */ {
                            auto dominant_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == dest_container.id; });
                            auto dominated_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == new_container.id; });
                            assert(dominated_it->v == dominant_it->v);
                        }
                        /* DBG */ set_dominated(c, new_container.id, dest_container.id);
                        assert(target(*new_container.pred_edge, g->graph) == target(*dest_container.pred_edge, g->graph));
                        invalid.push_back(new_container.id);
                        new_container_dominated = true;
                        break;
                    } else {
                        ++dest_unp_cnt_it;
                    }
                }
                
                if(unprocessed.at(dest_vertex).empty()) { unprocessed.erase(dest_vertex); }
                if(new_container_dominated) { continue; }
            }
            
            // If there are undominated labels at the destination vertex,
            // if any of them dominates the new label, then discard the new label;
            // if the new label dominates any of them, then discard them.
            if(undominated.find(dest_vertex) != undominated.end()) {
                auto dest_und_cnt_it = undominated.at(dest_vertex).begin();
                
                while(dest_und_cnt_it != undominated.at(dest_vertex).end()) {
                    const LblContainer<Lbl>& dest_container = *dest_und_cnt_it;
                    
                    if(new_container.label < dest_container.label && !(dest_container.label < new_container.label)) {
                        /* DBG */ {
                            auto dominated_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == dest_container.id; });
                            auto dominant_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == new_container.id; });
                            assert(dominated_it->v == dominant_it->v);
                        }
                        /* DBG */ set_dominated(c, dest_container.id, new_container.id);
                        assert(target(*new_container.pred_edge, g->graph) == target(*dest_container.pred_edge, g->graph));
                        invalid.push_back(dest_container.id);
                        undominated.at(dest_vertex).erase(dest_und_cnt_it++);
                    } else if(dest_container.label < new_container.label && !(new_container.label < dest_container.label)) {
                        /* DBG */ {
                            auto dominant_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == dest_container.id; });
                            auto dominated_it = std::find_if(c.begin(), c.end(), [&] (const auto& oc) { return oc.id == new_container.id; });
                            assert(dominated_it->v == dominant_it->v);
                        }
                        /* DBG */ set_dominated(c, new_container.id, dest_container.id);
                        assert(target(*new_container.pred_edge, g->graph) == target(*dest_container.pred_edge, g->graph));
                        invalid.push_back(new_container.id);
                        new_container_dominated = true;
                        break;
                    } else {
                        ++dest_und_cnt_it;
                    }
                }
                
                if(undominated.at(dest_vertex).empty()) { undominated.erase(dest_vertex); }
                if(new_container_dominated) { continue; }
            }
            
            typename ContainersSet<Lbl>::iterator new_inserted_it;
            bool new_inserted;
            
            // If we arrived up to here, it means that the new label is not dominated
            // by any existing label at the destination vertex, so we can place it in
            // the set of unprocessed labels at the destination vertex.
            if(unprocessed.find(dest_vertex) == unprocessed.end()) { unprocessed[dest_vertex] = ContainersSet<Lbl>(); }
            std::tie(new_inserted_it, new_inserted) = unprocessed.at(dest_vertex).insert(new_container);
            
            assert(new_inserted_it->pred_container);
            assert(new_inserted_it->pred_edge);
        }
    }
    
    // If there was no path leading to the end vertex, return an empty set of solutions
    if(undominated.find(end_v) == undominated.end()) { return std::vector<Solution>(); }
    
    // We now get the undominated labels at the end vertex
    const ContainersSet<Lbl>& pareto_optimal_containers = undominated.at(end_v);
    std::vector<Solution> pareto_optimal_solutions;
    pareto_optimal_solutions.reserve(pareto_optimal_containers.size());
    
    // And, for each of them, we reconstruct the corresponding optimal path
    for(const auto& oc : pareto_optimal_containers) {
        Path p;
        double reduced_cost = 0;
        const LblContainer<Lbl>* current = &oc;
        bool valid_path = true;
                
        while(current->pred_container != nullptr) {            
            if( !current->pred_edge ||
                current->predecessors.back() != current->pred_container->id ||
                std::find(invalid.begin(), invalid.end(), current->id) != invalid.end())
            {
                valid_path = false;
                break;
            }
            
            p.push_back(*current->pred_edge);
            reduced_cost += current->label.cost;
            
            if(current->pred_container->pred_container == nullptr) { reduced_cost += current->pred_container->label.cost; }
            
            current = current->pred_container;
        }
        
        if(valid_path) {            
            // And we add it to the set of solutions to return
            pareto_optimal_solutions.emplace_back(p, reduced_cost, g);
            if(!pareto_optimal_solutions.back().satisfies_capacity_constraints()) {
                std::cout << "VIOLATES CAPACITY CONSTRAINTS:" << std::endl;
                g->print_path(p, std::cout);
            }
        }
    }
    
    return pareto_optimal_solutions;
}

#endif