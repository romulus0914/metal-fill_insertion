CXX = g++
CXXFLAGS = -std=c++0x -g
CXXSOURCES = problem_c.cpp
CXXHEADERS = problem_c.h

TARGET = cada064

all::$(TARGET)

$(TARGET) : $(CXXSOURCES) $(CXXHEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -rf $(TARGET)
