CXX=		/usr/local/bin/g++-4.9

CPPFLAGS=	-I/Users/alberto/Applications/boost \
			-I/Users/alberto/Applications/cplex/include \
			-I/Users/alberto/Applications/concert/include \
			-I/Users/alberto/Applications/maersk \
			-std=c++11 -O0 -DIL_STD -g 

node.o: node.h port.h vessel_class.h node.cpp
	$(CXX) $(CPPFLAGS) -c -o node.o

graph.o: arc.h graph.h graph.cpp
	$(CXX) $(CPPFLAGS) -c -o graph.o
	
problem.o: problem.h problem.cpp
	$(CXX) $(CPPFLAGS) -c -o problem.o