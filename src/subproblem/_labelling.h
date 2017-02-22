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

#include "../base/graph.h"
#include "../column/solution.h"

namespace mvrp {
    using VisitablePorts = std::vector<std::pair<std::shared_ptr<Port>, PortType>>;

    class Label {
    public:
        const Graph& g;
        int pic; // Qty it's possible to pickup
        int del; // Qty it's possible to deliver
        int tdel; // Total qty delivered
        double cost;
    
        static constexpr double EPS = 0.001;
    
        Label(  const Graph& g,
                int pic,
                int del,
                double cost) :
                g{g},
                pic{pic},
                del{del},
                tdel{tdel},
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

    /* ============================================= */

    template<typename Lbl>
    class LblContainer {
    public:
        // In general, things that go inside std::set should be accessed via
        // const_iterator. The reason is that, if you accessed an element of
        // an ordered set, you could modify it in such a way that would invalidate
        // the order, and therefore the iterators.
        // However, in our case, there are times in which we want to access
        // LblContainers which are inside a set and we "promise" that this
        // will not invalidate anything. The things of a LblContainer that can
        // be accessed without problems, are marked as ``mutable'' below.
        
        Lbl label;
        
        const LblContainer* pred_container;
        mutable std::vector<const LblContainer*> succ_containers;
        
        const BGraph& g;
        boost::optional<const Edge> pred_edge;
        
        mutable bool dominated;
    
        LblContainer(   Lbl label,
                        const LblContainer* pred_container,
                        const BGraph& g,
                        const Edge pred_edge) :
                        label{label},
                        pred_container{pred_container},
                        succ_containers{},
                        g{g},
                        pred_edge{pred_edge},
                        dominated{false} {}
    
        LblContainer(   Lbl label,
                        const BGraph& g) :
                        label{label},
                        pred_container{nullptr},
                        succ_containers{},
                        g{g},
                        pred_edge{boost::none},
                        dominated{false} {}
                        
       void prepare_to_be_erased() const {
           if(pred_container != nullptr) {
               auto& c = pred_container->succ_containers;
               
               if(std::find(c.begin(), c.end(), this) == c.end()) {
                   std::cout << "Container: " << this << std::endl;
                   std::cout << "List of successors of my predecessor: ";
                   for(const auto& p : c) { std::cout << p << " "; }
                   std::cout << std::endl;
               }
               
               c.erase(std::remove(c.begin(), c.end(), this), c.end());
           }
       }
       
       void prepare_to_be_repaced_by(const LblContainer* replacement) const {
           if(pred_container != nullptr) {
               auto& c = pred_container->succ_containers;
               auto myself_in_parent = std::find(c.begin(), c.end(), this);
               assert(myself_in_parent != c.end());
               *myself_in_parent = replacement;
           }
       }
       
        void recursive_mark_dominated() const {
            dominated = true;
            for(const auto& successor : succ_containers) {
                successor->recursive_mark_dominated();
            }
        }
    };

    template<typename Lbl>
    struct LblContainerComp {
      bool operator()(const LblContainer<Lbl>& c1, const LblContainer<Lbl>& c2) const {
          if(c1.label.pic > c1.label.pic) { return true; }
          if(c1.label.del > c2.label.del) { return true; }
          return (c1.label.cost < c2.label.cost);
      }  
    };

    template<typename Lbl>
    class ContainersSet {
        std::set<LblContainer<Lbl>, LblContainerComp<Lbl>> set;
    
    public:
        using iterator = typename std::set<LblContainer<Lbl>, LblContainerComp<Lbl>>::iterator;
    
        ContainersSet(std::initializer_list<LblContainer<Lbl>> l) : set{l} {}
        ContainersSet() {}
    
        friend auto begin(const ContainersSet& s) { return s.set.begin(); }
        friend auto end(const ContainersSet& s) { return s.set.end(); }
    
        bool has_undominated_labels() const {
            if(set.empty()) { return false; }
            if(std::any_of(set.begin(), set.end(), [] (const LblContainer<Lbl>& c) { return !c.dominated; })) { return true; }
            return false;
        }
    
        auto first_undominated_container() const {
            for(auto it = set.begin(); it != set.end(); ++it) { if(!it->dominated) { return it; } }
            return set.end();
        }
    
        const iterator begin() const { return set.begin(); }
        const iterator end() const { return set.end(); }
        iterator begin() { return set.begin(); }
        iterator end() { return set.end(); }
        auto insert(LblContainer<Lbl> l) { return set.insert(l); }
        auto erase(const iterator& l) { return set.erase(l); }
        auto size() const { return set.size(); }
        auto empty() const { return set.empty(); }
        void mark_dominated(iterator l) {
            assert(l != set.end());
            l->recursive_mark_dominated();
        }
    };

    template<typename Lbl>
    class VertexContainersMap {
        std::map<Vertex, ContainersSet<Lbl>> map;
    
    public:
        using iterator = typename std::map<Vertex, ContainersSet<Lbl>>::iterator;
    
        friend auto begin(const VertexContainersMap& m) { return m.map.begin(); }
        friend auto end(const VertexContainersMap& m) { return m.map.end(); }
    
        bool has_undominated_labels() const {
            if(map.empty()) { return false; }
            for(const auto& vs : map) { if(!vs.second.empty()) { return true; } }
            return false;
        }
    
        auto first_with_undominated_container() const {
            for(auto it = map.begin(); it != map.end(); ++it) {
                if(std::any_of(it->second.begin(), it->second.end(),
                    [] (const auto& container) -> bool { return !container.dominated; }
                )) { return it; }
            }
            return map.end();
        }
    
        const iterator begin() const { return map.begin(); }
        const iterator end() const { return map.end(); }
        iterator begin() { return map.begin(); }
        iterator end() { return map.end(); }
        auto find(const Vertex& v) const { return map.find(v); }
        auto erase(const Vertex& v) { return map.erase(v); }
        auto empty() const { return map.empty(); }
        const ContainersSet<Lbl>& at(const Vertex& v) const { return map.at(v); }
        ContainersSet<Lbl>& at(const Vertex& v) { return map.at(v); }
        ContainersSet<Lbl>& operator[](const Vertex& v) { return map[v]; }
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

    template<typename Lbl, typename LblExt>
    std::vector<Solution> LabellingAlgorithm<Lbl, LblExt>::solve(Vertex start_v, Vertex end_v, Lbl start_label, LblExt extension) const {
        VertexContainersMap<Lbl> undominated;
        VertexContainersMap<Lbl> unprocessed;
    
        // In the beginning we only have the starting label, as an unprocessed label at the starting vertex
        unprocessed[start_v] = { LblContainer<Lbl>(start_label, g->graph) };
    
        // While there are unprocessed labels...
        while(unprocessed.has_undominated_labels()) {
            // Get the first vertex with unprocessed labels
            auto any_set_it = unprocessed.first_with_undominated_container();
            Vertex cur_vertex = any_set_it->first;
            const ContainersSet<Lbl>& containers_at_cur_vertex = any_set_it->second;
        
            // Get the first unprocessed label at the selected vertex
            assert(containers_at_cur_vertex.has_undominated_labels());
            auto any_cnt_it = containers_at_cur_vertex.first_undominated_container();
        
            // Make a copy of the chosen label
            auto cur_container = LblContainer<Lbl>(*any_cnt_it);
        
            typename ContainersSet<Lbl>::iterator cur_inserted_it;
            bool cur_inserted;
        
            // Insert the current label in undominated
            if(undominated.find(cur_vertex) == undominated.end()) { undominated[cur_vertex] = ContainersSet<Lbl>(); }
            std::tie(cur_inserted_it, cur_inserted) = undominated.at(cur_vertex).insert(cur_container);
            assert(!cur_inserted_it->pred_container || cur_inserted_it->pred_edge);
        
            // Remove the label from unprocessed
            any_cnt_it->prepare_to_be_repaced_by(&(*cur_inserted_it));
            unprocessed.at(cur_vertex).erase(any_cnt_it);
            if(unprocessed.at(cur_vertex).empty()) { unprocessed.erase(cur_vertex); }
                
            // Try to expand the current label along all out-edges departing from the current vertex
            for(auto oe = out_edges(cur_vertex, g->graph); oe.first != oe.second; ++oe.first) {
                Edge e = *oe.first;
                Vertex dest_vertex = target(e, g->graph);
            
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
                    
                        if(new_container.label < dest_container.label && !(dest_container.label < new_container.label)) {
                            assert(target(*new_container.pred_edge, g->graph) == target(*dest_container.pred_edge, g->graph));
                            dest_container.prepare_to_be_erased();
                            unprocessed.at(dest_vertex).erase(dest_unp_cnt_it++);
                        } else if(dest_container.label < new_container.label && !(new_container.label < dest_container.label)) {
                            assert(target(*new_container.pred_edge, g->graph) == target(*dest_container.pred_edge, g->graph));
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
                    
                        if(!dest_container.dominated && new_container.label < dest_container.label && !(dest_container.label < new_container.label)) {
                            assert(target(*new_container.pred_edge, g->graph) == target(*dest_container.pred_edge, g->graph));
                            undominated.at(dest_vertex).mark_dominated(dest_und_cnt_it++);
                        } else if(!dest_container.dominated && dest_container.label < new_container.label && !(new_container.label < dest_container.label)) {
                            assert(target(*new_container.pred_edge, g->graph) == target(*dest_container.pred_edge, g->graph));
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
                
                // Add the new label to the set of successors of its predecessor.
                new_inserted_it->pred_container->succ_containers.push_back(&(*new_inserted_it));
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
                p.push_back(*current->pred_edge);
                reduced_cost += current->label.cost;
            
                if(current->pred_container->pred_container == nullptr) { reduced_cost += current->pred_container->label.cost; }
            
                current = current->pred_container;
            }
        
            // And we add it to the set of solutions to return
            pareto_optimal_solutions.emplace_back(p, reduced_cost, g);
        }
    
        return pareto_optimal_solutions;
    }
}

#endif