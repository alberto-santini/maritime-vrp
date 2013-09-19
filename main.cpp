//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <base/base.h>
#include <base/graph.h>
#include <preprocessing/graph_generator.h>
#include <preprocessing/problem_data.h>

int main() {
    ProblemData data = ProblemData();
    Graph g;
    
    GraphGenerator::create_graph(data, data.vessel_classes.front(), g);
    
    return 0;
}

