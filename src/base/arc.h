//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#ifndef ARC_H
#define ARC_H

class Arc {
public:
    double  cost;
    
    double  bunker_costs;
    double  tc_costs;
    double  movement_costs;
    double  port_costs;
    double  revenue;
    
    double  length;
    int     boost_edge_id;
    
    Arc() {}
    Arc(double bunker_costs,
        double tc_costs,
        double movement_costs,
        double port_costs,
        double revenue,
        double length) :
        cost(bunker_costs + tc_costs + movement_costs + port_costs - revenue),
        bunker_costs(bunker_costs),
        tc_costs(tc_costs),
        movement_costs(movement_costs),
        port_costs(port_costs),
        revenue(revenue),
        length(length) { boost_edge_id = 0; }
};

#endif
