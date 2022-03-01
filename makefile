CXX = g++
CXXFLAGS = -Wall -g

proj0: mytest.cpp mqueue.cpp mqueue.h
	$(CXX) $(CXXFLAGS) mytest.cpp mqueue.cpp mqueue.h -o proj03

run:
	./proj03

val:
	valgrind proj03 -s --track-origins=yes
