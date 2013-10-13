//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef CYCLE_H
#define CYCLE_H

#include <base/base.h>
#include <base/graph.h>

typedef vector<pair<Path, const std::shared_ptr<const Graph>>> Cycles;

namespace Cycle {
    Path shortest_cycle(const Path& route, const std::shared_ptr<const Graph> g);
    void print_cycle(const Path& cycle, const std::shared_ptr<const Graph> g, ostream& out = cerr);
}

#endif