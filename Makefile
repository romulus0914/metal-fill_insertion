CXX = g++
CXXFLAGS = -std=c++11 -g
CXXSOURCES = problem_c.cpp
CXXHEADERS = problem_c.h capacitance.h

TARGET = problem_c

all::$(TARGET)

$(TARGET) : $(CXXSOURCES) $(CXXHEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -rf $(TARGET) $(TARGET).dSYM $(CXXOBJS)
