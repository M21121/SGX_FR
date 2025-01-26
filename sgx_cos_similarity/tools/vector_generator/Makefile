# tools/vector_generator/Makefile
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I../../

SOURCES = main.cpp vector_generator.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = vector_generator

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
