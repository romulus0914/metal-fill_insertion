CXX = g++
CXXFLAGS = -std=c++11 -g

CXXSOURCES = problem_c.cpp

TARGET = problem_c

all::$(TARGET)

$(TARGET) : $(CXXSOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -rf $(TARGET) $(TARGET).dSYM
