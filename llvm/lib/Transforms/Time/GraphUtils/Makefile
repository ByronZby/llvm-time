CXX = g++

IFLAGS =

CXXWARNS = -pedantic -Wall -Wextra -Wcast-align -Wcast-qual \
		   -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self \
		   -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs \
           -Wnoexcept -Woverloaded-virtual -Wredundant-decls -Wshadow \
		   -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef 

CXXFLAGS = -g -std=c++17 -Werror -Wfatal-errors $(CXXWARNS)  $(IFLAGS)

LDFLAGS = -g
LDLIBS =
INCLUDES = $(shell echo *.h)

%.o: %.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: local-test


local-test: GraphTest.o PtrGraph.o GraphGenerator.o TopologicalOrder.o Cycle.o SingleEntrySingleExitDAG.o PathProfiler.o
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -f local-test *.o core.*
