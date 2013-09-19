CXX=			/usr/local/bin/g++-4.9

CPPFLAGS=	-I/Users/alberto/Applications/boost \
					-I/Users/alberto/Applications/cplex/include \
					-I/Users/alberto/Applications/concert/include \
					-I/Users/alberto/Applications/maersk \
					-std=c++11 -O3 -DIL_STD

problem_data.o: problem_data.h problem_data.cpp
	$(CXX) $(CPPFLAGS) -c -o problem_data.o

graph_generator.o: graph_generator.h graph_generator.cpp
	$(CXX) $(CPPFLAGS) -c -o graph_generator.o