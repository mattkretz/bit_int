check: test.cpp bit_int.hpp
	$(CXX) -std=c++26 -Wall -Wextra -c -o test.o test.cpp
	@rm test.o

.PHONY: check
