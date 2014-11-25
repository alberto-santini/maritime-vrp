//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef ARC_H
#define ARC_H

class Arc {
public:
    double  cost;
    int     boost_edge_id;
    
    Arc() {}
    Arc(double cost) : cost(cost) { boost_edge_id = 0; }
};

#endif
