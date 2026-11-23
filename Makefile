CXXFLAGS=-std=c++29 -Wall -Wextra # $(CLANG_FLAGS)

check: test.cpp bit_int.hpp
	$(CXX) $(CXXFLAGS) -c -o test.o test.cpp
	@rm test.o

check2: test2.cpp integer.hpp
	$(CXX) $(CXXFLAGS) -c -o test2.o test2.cpp
	@rm test2.o

.PHONY: check check2 help

help:
	echo "... check"
	echo "... check2"
