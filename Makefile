# Simple Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Iinclude -O2
SRCS = $(wildcard src/*.cpp)
TARGET = cache_simulator

all:
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
