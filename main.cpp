//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <iostream>

#include <branching/bb_tree.h>

int main(int argc, char* argv[]) {
    srand(12345);
    
    if(argc != 3) {
        std::cout << "Usage: ./feeder <params_file> <data_file>" << std::endl;
        return -1;
    }
    
    BBTree tree = BBTree(argv[1], argv[2]);
    tree.explore_tree();
    
    return 0;
}