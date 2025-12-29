# Test harness Makefile for stereo 3D validation
# Run: make -C tests test-stereo

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -g -O0
TARGET = test_stereo_math
SOURCES = test_stereo_math.cpp

.PHONY: all clean test-stereo

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) -lm

test-stereo: $(TARGET)
	@echo "Running stereo math tests..."
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).o

