CXX=			/usr/local/bin/g++-4.9

CPPFLAGS=	-I/Users/alberto/Applications/boost \
					-I/Users/alberto/Applications/cplex/include \
					-I/Users/alberto/Applications/concert/include \
					-I/Users/alberto/Applications/maersk \
					-std=c++11 -O3 -DIL_STD

knapsack.o: knapsack.h knapsack.cpp
	$(CXX) $(CPPFLAGS) -c -o knapsack.o