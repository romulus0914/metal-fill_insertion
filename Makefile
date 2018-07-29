CXX = g++
CXXFLAGS = -std=c++11 -g
CXXSOURCES = problem_c.cpp
CXXHEADERS = problem_c.h capacitance.h

TARGET = cada064

all::$(TARGET)

$(TARGET) : $(CXXSOURCES) $(CXXHEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -rf $(TARGET) $(TARGET).dSYM $(CXXOBJS)
