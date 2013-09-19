//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <base/base.h>
#include <base/graph.h>
#include <base/problem.h>

int main() {
    Problem prob = Problem();
    
    for(auto& vg : prob.graphs) {
        cout << vg.second.name << " has " << num_vertices(vg.second.graph) << " vertices and " << num_edges(vg.second.graph) << " edges" << endl;
    }

    return 0;
}