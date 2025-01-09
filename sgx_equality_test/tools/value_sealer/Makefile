CC = g++
CFLAGS = -fPIC -Wno-attributes
SGX_SDK = /opt/intel/sgxsdk
INCLUDE_PATHS = -I$(SGX_SDK)/include -I../../common
LIBRARY_PATHS = -L$(SGX_SDK)/lib64
LIBRARIES = -lsgx_urts -lpthread

all: value_sealer

value_sealer: value_sealer.cpp
	$(CC) value_sealer.cpp -o value_sealer $(CFLAGS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(LIBRARIES)

clean:
	rm -f value_sealer
