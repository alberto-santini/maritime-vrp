CXX=		/usr/local/bin/g++-4.9

CPPFLAGS=	-I/Users/alberto/Applications/boost \
			-I/Users/alberto/Applications/cplex/include \
			-I/Users/alberto/Applications/concert/include \
			-I/Users/alberto/Applications/maersk \
			-std=c++11 -O3 -DIL_STD  

mp_solver.o: mp_linear_solution.h mp_solver.h mp_solver.cpp
	$(CXX) $(CPPFLAGS) -c -o mp_solver.o