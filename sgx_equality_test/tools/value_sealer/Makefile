CXX = g++
CXXFLAGS = -Wall -std=c++11
LDFLAGS = -lcrypto -lssl

all: value_sealer

value_sealer: value_sealer.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f value_sealer
