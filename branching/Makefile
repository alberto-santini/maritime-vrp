CXX=		/usr/local/bin/g++-4.9

CPPFLAGS=	-I/Users/alberto/Applications/boost \
			-I/Users/alberto/Applications/cplex/include \
			-I/Users/alberto/Applications/concert/include \
			-I/Users/alberto/Applications/maersk \
			-std=c++11 -O0 -DIL_STD -g

bb_node.o: bb_node.h bb_node.cpp
	$(CXX) $(CPPFLAGS) -c -o bb_node.o