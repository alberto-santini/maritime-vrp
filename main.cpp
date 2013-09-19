//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <base/base.h>
#include <base/graph.h>
#include <base/problem.h>

int main() {
    Problem p = Problem();
    
    for(Graph& g : p.graphs) {
        cout << g.name << " has " << num_vertices(g.graph) << " vertices and " << num_edges(g.graph) << " edges" << endl;
    }
}

