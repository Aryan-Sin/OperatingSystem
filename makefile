CXX = g++
CXXFLAGS = -Wall -g

proj04: mytest.cpp hash.cpp hash.h file.cpp file.h
	$(CXX) $(CXXFLAGS) mytest.cpp hash.cpp hash.h file.cpp file.h -o proj04

run:
	./proj04

val:
	valgrind -s --track-origins=yes proj04
