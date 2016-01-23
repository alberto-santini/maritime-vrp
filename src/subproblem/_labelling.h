//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef LABELLING_H
#define LABELLING_H

#include <map>
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <unordered_set>
#include <boost/optional.hpp>
#include <boost/functional/hash.hpp>

#include <base/graph.h>
#include <column/solution.h>

using VisitablePorts = std::vector<std::pair<std::shared_ptr<Port>, PickupType>>;

class Label {
public:
    std::shared_ptr<const Graph> g;
    int pic;
    int del;
    double cost;
    
    static constexpr double EPS = 0.00001;
    
    Label(  std::shared_ptr<const Graph> g,
            int pic,
            int del,
            double cost = 0) :
            g{g},
            pic{pic},
            del{del},
            cost{cost} {}
};

class ElementaryLabel : public Label {
public:
    VisitablePorts por;
    
    ElementaryLabel(std::shared_ptr<const Graph> g,
                    int pic,
                    int del,
                    double cost,
                    VisitablePorts por) : 
                    Label{g, pic, del, cost},
                    por{por} {}
};

class LabelExtender {
public:
    const ErasedEdges& erased;
    
    LabelExtender(const ErasedEdges& erased) : erased{erased} {}

    boost::optional<Label> operator()(const BGraph& graph, const Label& label, const Edge& e) const;
    boost::optional<ElementaryLabel> operator()(const BGraph& graph, const ElementaryLabel& label, const Edge& e) const;
};

template<typename Lbl>
class LblContainer {
public:
    Lbl label;
    const LblContainer* pred_container;
    const Edge* pred_edge;
    
    LblContainer(   Lbl label,
                    const LblContainer* pred_container,
                    const Edge* pred_edge) :
                    label{label},
                    pred_container{pred_container},
                    pred_edge{pred_edge} {}
    
    LblContainer(   Lbl label) :
                    label{label},
                    pred_container{nullptr},
                    pred_edge{nullptr} {}
};

template<typename Lbl>
struct LblContainerHash {
    std::size_t operator()(const LblContainer<Lbl>& c) const {
        std::size_t seed = 0;
        boost::hash_combine(seed, std::hash<const LblContainer<Lbl>*>()(c.pred_container));
        boost::hash_combine(seed, std::hash<const Edge*>()(c.pred_edge));
        return seed;
    }
};

template<typename Lbl>
bool operator==(const LblContainer<Lbl>& c1, const LblContainer<Lbl>& c2) {
    if(c1.pred_edge != c2.pred_edge) { return false; }
    if(c1.pred_container != c2.pred_container) { return false; }
    return c1.label == c2.label;
}

template<typename Lbl, typename LblExt>
class LabellingAlgorithm {
    std::shared_ptr<const Graph> g;
    
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

template<typename Lbl, typename LblExt>
std::vector<Solution> LabellingAlgorithm<Lbl, LblExt>::solve(Vertex start_v, Vertex end_v, Lbl start_label, LblExt extension) const {
    std::map<Vertex, std::unordered_set<LblContainer<Lbl>, LblContainerHash<Lbl>>> undominated;
    std::map<Vertex, std::unordered_set<LblContainer<Lbl>, LblContainerHash<Lbl>>> unprocessed;
    
    // In the beginning we only have the starting label, as an unprocessed label at the starting vertex
    unprocessed[start_v] = { LblContainer<Lbl>(start_label) };
    
    // While there are unprocessed labels...
    while(!unprocessed.empty()) {
        // Get the first vertex with unprocessed labels
        auto any_set_it = unprocessed.begin();
        const Vertex& cur_vertex = any_set_it->first;
        const std::unordered_set<LblContainer<Lbl>, LblContainerHash<Lbl>>& containers_at_cur_vertex = any_set_it->second;
        
        // Get the first unprocessed label at the selected vertex
        auto any_cnt_it = containers_at_cur_vertex.begin();
        
        // Move the label from unprocessed to undominated
        const LblContainer<Lbl> cur_container = *any_cnt_it;
        if(undominated.find(cur_vertex) == undominated.end()) {
            undominated[cur_vertex] = { cur_container };
        } else {
            undominated.at(cur_vertex).insert(cur_container);
        }
        unprocessed.at(cur_vertex).erase(any_cnt_it);
        
        // If there is no unprocessed label at the current vertex, clear the corresponding map entry
        if(unprocessed.at(cur_vertex).size() == 0u) {
            unprocessed.erase(cur_vertex);
        }
        
        // Try to expand the current label along all out-edges departing from the current vertex
        for(auto oe = out_edges(cur_vertex, g->graph); oe.first != oe.second; ++oe.first) {
            const Edge& e = *oe.first;
            const Vertex& dest_vertex = target(e, g->graph);
            
            // Call to the extension function
            auto new_label = extension(g->graph, cur_container.label, e);
            
            // Extension didn't succeed: skip the rest
            if(!new_label) { continue; }
            
            // Extension succeeded! Create a container for the new label
            auto new_container = LblContainer<Lbl>(*new_label, &cur_container, &e);
            bool new_container_dominated = false;
            
            // If there are unprocessed labels at the destination vertex,
            // if any of them dominates the new label, then discard the new label;
            // if the new label dominates any of them, then discard them.
            if(unprocessed.find(dest_vertex) != unprocessed.end()) {
                auto dest_unp_cnt_it = unprocessed.at(dest_vertex).begin();
                
                while(dest_unp_cnt_it != unprocessed.at(dest_vertex).end()) {
                    const LblContainer<Lbl>& dest_container = *dest_unp_cnt_it;
                    
                    if(cur_container.label < dest_container.label) {
                        unprocessed.at(dest_vertex).erase(dest_unp_cnt_it++);
                    } else if(dest_container.label < cur_container.label) {
                        new_container_dominated = true;
                        break;
                    } else {
                        ++dest_unp_cnt_it;
                    }
                }
                
                if(new_container_dominated) { continue; }
            }
            
            // If there are undominated labels at the destination vertex,
            // if any of them dominates the new label, then discard the new label;
            // if the new label dominates any of them, then discard them.
            if(undominated.find(dest_vertex) != undominated.end()) {
                auto dest_und_cnt_it = undominated.at(dest_vertex).begin();
                
                while(dest_und_cnt_it != undominated.at(dest_vertex).end()) {
                    const LblContainer<Lbl>& dest_container = *dest_und_cnt_it;
                    
                    if(cur_container.label < dest_container.label) {
                        undominated.at(dest_vertex).erase(dest_und_cnt_it++);
                    } else if(dest_container.label < cur_container.label) {
                        new_container_dominated = true;
                        break;
                    } else {
                        ++dest_und_cnt_it;
                    }
                }
                
                if(new_container_dominated) { continue; }
            }
            
            // If we arrived up to here, it means that the new label is not dominated
            // by any existing label at the destination vertex, so we can place it in
            // the set of unprocessed labels at the destination vertex.
            if(unprocessed.find(dest_vertex) == unprocessed.end()) {
                unprocessed[dest_vertex] = { new_container };
            } else {
                unprocessed.at(dest_vertex).insert(new_container);
            }
            
        }
    }
    
    // If there was no path leading to the end vertex, return an empty set of solutions
    if(undominated.find(end_v) == undominated.end()) {
        return std::vector<Solution>();
    }
    
    // We now get the undominated labels at the end vertex
    const std::unordered_set<LblContainer<Lbl>, LblContainerHash<Lbl>>& pareto_optimal_containers = undominated.at(end_v);
    std::vector<Solution> pareto_optimal_solutions;
    pareto_optimal_solutions.reserve(pareto_optimal_containers.size());
    
    // And, for each of them, we reconstruct the corresponding optimal path
    for(const auto& oc : pareto_optimal_containers) {
        Path p;
        double reduced_cost = 0;
        const LblContainer<Lbl>* current = &oc;
        
        while(current->pred_container != nullptr) {
            assert(current->pred_edge != nullptr);
            
            p.push_back(*current->pred_edge);
            reduced_cost += current->label.cost;
            current = current->pred_container;
        }
        
        // We reverse it, since we built it from the end to the start
        std::reverse(p.begin(), p.end());
        
        // And we add it to the set of solutions to return
        pareto_optimal_solutions.emplace_back(p, reduced_cost, g);
    }
    
    return pareto_optimal_solutions;
}

#endif