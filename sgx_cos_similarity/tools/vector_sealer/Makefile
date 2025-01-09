CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -I../../common

SRCS = vector_sealer.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = vector_sealer

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
