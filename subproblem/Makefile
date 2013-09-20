CXX=		/usr/local/bin/g++-4.9

CPPFLAGS=	-I/Users/alberto/Applications/boost \
			-I/Users/alberto/Applications/cplex/include \
			-I/Users/alberto/Applications/concert/include \
			-I/Users/alberto/Applications/maersk \
			-std=c++11 -O3 -DIL_STD

labelling.o: id_maps.g labelling.h labelling.cpp
	$(CXX) $(CPPFLAGS) -c -o labelling.o

heuristics_solver.o: heuristics_solver.h heuristics_solver.cpp
	$(CXX) $(CPPFLAGS) -c -o heuristics_solver.o