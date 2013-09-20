//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef ID_MAPS_H
#define ID_MAPS_H

#include <base/base.h>
#include <base/graph.h>

class NodeIdFunctor {
    const Graph& g;
    
public:
    typedef int result_type;
  
    NodeIdFunctor(const Graph& g) : g(g) {}
  
    result_type operator()(const Vertex v) const {
        return g.graph[v]->boost_vertex_id;
    }
};

class ArcIdFunctor {
    const Graph& g;

public:
    typedef int result_type;
  
    ArcIdFunctor(const Graph& g) : g(g) {}
  
    result_type operator()(const Edge e) const {
        return g.graph[e]->boost_edge_id;
    }
};

template<typename Fun, typename Arg> class FunctionPropertyMap {
    const Fun& f;
  
public:
    typedef typename boost::result_of<Fun(Arg)>::type value_type;
  
    explicit FunctionPropertyMap(const Fun& f) : f(f) {}
  
    friend value_type get(const FunctionPropertyMap& pm, const Arg& arg) {
        return pm.f(arg);
    }
    
    value_type operator[](const Arg& arg) const {
        return f(arg);
    }
};

namespace boost{
    template<typename Fun, typename Arg> struct property_traits<FunctionPropertyMap<Fun, Arg>> {
        typedef typename boost::result_of<Fun(Arg)>::type value_type;
        typedef value_type reference;
        typedef Arg key_type;
        typedef readable_property_map_tag category;
    };
}

template<typename Arg, typename Fun> FunctionPropertyMap<Fun, Arg> make_property_map(const Fun& f) {
    return FunctionPropertyMap<Fun, Arg>(f);
}

#endif