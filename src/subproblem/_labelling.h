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
    const Graph& g;
    int pic;
    int del;
    double cost;
    
    static constexpr double EPS = 0.00001;
    
    Label(  const Graph& g,
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
    
    ElementaryLabel(const Graph& g,
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
    const BGraph& g;
    boost::optional<const Edge> pred_edge;
    
    LblContainer(   Lbl label,
                    const LblContainer* pred_container,
                    const BGraph& g,
                    const Edge pred_edge) :
                    label{label},
                    pred_container{pred_container},
                    g{g},
                    pred_edge{pred_edge} {}
    
    LblContainer(   Lbl label,
                    const BGraph& g) :
                    label{label},
                    pred_container{nullptr},
                    g{g},
                    pred_edge{boost::none} {}
};

template<typename Lbl>
struct LblContainerHash {
    std::size_t operator()(const LblContainer<Lbl>& c) const {
        std::size_t seed = 0;
        boost::hash_combine(seed, std::hash<const LblContainer<Lbl>*>()(c.pred_container));
        if(c.pred_edge) {
            boost::hash_combine(seed, std::hash<int>()(c.g[*c.pred_edge]->boost_edge_id));
        } else {
            boost::hash_combine(seed, std::hash<int>()(-1));
        }
        return seed;
    }
};

template<typename Lbl>
using ContainersSet = std::unordered_set<LblContainer<Lbl>, LblContainerHash<Lbl>>;

template<typename Lbl>
using VertexContainersMap = std::map<Vertex, ContainersSet<Lbl>>;

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

template<typename Lbl>
void print_map(const VertexContainersMap<Lbl>& m, const Graph& g) {
    for(const auto& vertex_set : m) {
        const Vertex& v = vertex_set.first;
        const ContainersSet<Lbl>& s = vertex_set.second;
        
        std::cout << *g.graph[v] << " => " << std::endl;
        
        for(const auto& container : s) {
            std::cout << "\t" << container.label << std::endl;
        }
    }
}

template<typename Lbl, typename LblExt>
std::vector<Solution> LabellingAlgorithm<Lbl, LblExt>::solve(Vertex start_v, Vertex end_v, Lbl start_label, LblExt extension) const {
    VertexContainersMap<Lbl> undominated;
    VertexContainersMap<Lbl> unprocessed;
    
    // In the beginning we only have the starting label, as an unprocessed label at the starting vertex
    unprocessed[start_v] = { LblContainer<Lbl>(start_label, g->graph) };
        
    // While there are unprocessed labels...
    while(!unprocessed.empty()) {
        // Get the first vertex with unprocessed labels
        auto any_set_it = unprocessed.begin();
        const Vertex& cur_vertex = any_set_it->first;
        const ContainersSet<Lbl>& containers_at_cur_vertex = any_set_it->second;
        
        // Get the first unprocessed label at the selected vertex
        assert(!containers_at_cur_vertex.empty());
        auto any_cnt_it = containers_at_cur_vertex.begin();
        
        // Make a copy of the chosen label
        auto cur_container = LblContainer<Lbl>(*any_cnt_it);
        
        typename ContainersSet<Lbl>::iterator cur_inserted_it;
        bool cur_inserted;
        
        // Insert the label in undominated
        if(undominated.find(cur_vertex) == undominated.end()) { undominated[cur_vertex] = ContainersSet<Lbl>(); }
        std::tie(cur_inserted_it, cur_inserted) = undominated.at(cur_vertex).insert(cur_container);
        
        assert(!cur_inserted_it->pred_container || cur_inserted_it->pred_edge);
        
        // Remove the label from unprocessed
        unprocessed.at(cur_vertex).erase(any_cnt_it);
        
        // If there is no unprocessed label at the current vertex, clear the corresponding map entry
        if(unprocessed.at(cur_vertex).empty()) { unprocessed.erase(cur_vertex); }
                
        // Try to expand the current label along all out-edges departing from the current vertex
        for(auto oe = out_edges(cur_vertex, g->graph); oe.first != oe.second; ++oe.first) {
            const Edge& e = *oe.first;
            const Vertex& dest_vertex = target(e, g->graph);
            
            // Call to the extension function
            auto new_label = extension(g->graph, cur_container.label, e);
            
            // Extension didn't succeed: skip the rest
            if(!new_label) { continue; }
            
            // Extension succeeded! Create a container for the new label
            auto new_container = LblContainer<Lbl>(*new_label, &(*cur_inserted_it), g->graph, e);
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
                    
                    if(cur_container.label < dest_container.label) {
                        undominated.at(dest_vertex).erase(dest_und_cnt_it++);
                    } else if(dest_container.label < cur_container.label) {
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
                
        while(current->pred_container != nullptr) {
            if(current->pred_edge) {
                std::cout << "Current container:" << std::endl;
                std::cout << "\tLabel: " << current->label << std::endl;
                std::cout << "\tEdge: " << *g->graph[source(*current->pred_edge, g->graph)];
                std::cout << " -> " << *g->graph[target(*current->pred_edge, g->graph)] << std::endl;
                std::cout << "\tPred label: " << current->pred_container->label << std::endl;
            } else {
                std::cout << "PROBLEM!" << std::endl;
                std::cout << "Current container:" << std::endl;
                std::cout << "\tLabel: " << current->label << std::endl;
                std::cout << "\tEdge: boost::none" << std::endl;
                std::cout << "\tPred label: " << current->pred_container->label << std::endl;
                assert(false);
            }
                
            p.push_back(*current->pred_edge);
            reduced_cost += current->label.cost;
            current = current->pred_container;
        }
        
        std::cout << "==================" << std::endl << std::endl;
        
        // We reverse it, since we built it from end to start
        std::reverse(p.begin(), p.end());
        
        // And we add it to the set of solutions to return
        pareto_optimal_solutions.emplace_back(p, reduced_cost, g);
    }
    
    return pareto_optimal_solutions;
}

#endif