SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
SGX_DEBUG ?= 1
SGX_PRERELEASE ?= 0
SGX_SWITCHLESS ?= 0

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

SGX_COMMON_CFLAGS := -m64 -O2
SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r

ifeq ($(SGX_DEBUG), 1)
	SGX_COMMON_CFLAGS += -O0 -g
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

SGX_COMMON_FLAGS += -Wall -Wextra -Winit-self -Wpointer-arith -Wreturn-type \
					-Waddress -Wsequence-point -Wformat-security \
					-Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow \
					-Wcast-align -Wcast-qual -Wconversion -Wredundant-decls
SGX_COMMON_CFLAGS := $(SGX_COMMON_FLAGS) -Wjump-misses-init -Wstrict-prototypes \
					 -Wunsuffixed-float-constants
SGX_COMMON_CXXFLAGS := $(SGX_COMMON_FLAGS) -Wnon-virtual-dtor -std=c++11

App_Cpp_Files := App/CosineApp.cpp
App_Include_Paths := -IApp -I$(SGX_SDK)/include -Icommon

App_C_Flags := -fPIC -Wno-attributes $(App_Include_Paths)
App_Cpp_Flags := $(App_C_Flags) $(SGX_COMMON_CXXFLAGS)

App_Link_Flags := -L$(SGX_SDK)/lib64 \
	-lsgx_urts -lpthread -lm

Enclave_Cpp_Files := Enclave/CosineEnclave.cpp
Enclave_Include_Paths := -IEnclave -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc \
						-I$(SGX_SDK)/include/libcxx -Icommon

Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector \
				   $(Enclave_Include_Paths)
Enclave_Cpp_Flags := $(Enclave_C_Flags) $(SGX_COMMON_CXXFLAGS) -nostdinc++

Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs \
					  -nostartfiles -L$(SGX_SDK)/lib64 \
					  -Wl,--whole-archive \
					  -lsgx_trts \
					  -Wl,--no-whole-archive \
					  -Wl,--start-group \
					  -lsgx_tstdc \
					  -lsgx_tcxx \
					  -lsgx_tkey_exchange \
					  -lsgx_tcrypto \
					  -lsgx_tservice \
					  -Wl,--end-group \
					  -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
					  -Wl,-pie,-eenclave_entry -Wl,--export-dynamic \
					  -Wl,--defsym,__ImageBase=0 -Wl,--gc-sections

ifeq ($(SGX_SWITCHLESS), 1)
	App_Link_Flags += -Wl,--whole-archive -lsgx_uswitchless -Wl,--no-whole-archive
	Enclave_Link_Flags += -lsgx_tswitchless
endif

.PHONY: all clean

all: sgx_cosine_sim.signed.so sgx_cos_similarity

######## EDL Objects ########
Enclave/CosineEnclave_t.h Enclave/CosineEnclave_t.c: Enclave/CosineEnclave.edl
	@echo "Generating trusted files..."
	@cd Enclave && $(SGX_EDGER8R) --trusted CosineEnclave.edl --search-path $(SGX_SDK)/include --search-path .
	@echo "Generated files in Enclave directory:"
	@ls -l Enclave/

App/CosineEnclave_u.h App/CosineEnclave_u.c: Enclave/CosineEnclave.edl
	@echo "Generating untrusted files..."
	@cd App && $(SGX_EDGER8R) --untrusted ../Enclave/CosineEnclave.edl --search-path $(SGX_SDK)/include --search-path ../Enclave
	@echo "Generated files in App directory:"
	@ls -l App/

######## App Objects ########
App/CosineEnclave_u.o: App/CosineEnclave_u.c App/CosineEnclave_u.h
	@echo "Compiling App/CosineEnclave_u.c"
	@echo "Current directory: $$(pwd)"
	@$(CC) $(App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

App/%.o: App/%.cpp App/CosineEnclave_u.h
	@echo "Compiling $<"
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

sgx_cos_similarity: App/CosineEnclave_u.o $(App_Cpp_Files:.cpp=.o)
	@echo "Linking sgx_cos_similarity"
	@$(CXX) $^ -o $@ $(App_Link_Flags)
	@echo "LINK =>  $@"

######## Enclave Objects ########
Enclave/CosineEnclave_t.o: Enclave/CosineEnclave_t.c Enclave/CosineEnclave_t.h
	@echo "Attempting to compile CosineEnclave_t.c"
	@echo "Current directory: $$(pwd)"
	@echo "Target file: $@"
	@echo "Source file location check:"
	@ls -la Enclave/CosineEnclave_t.c || echo "Source file not found"
	@$(CC) $(Enclave_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

Enclave/%.o: Enclave/%.cpp Enclave/CosineEnclave_t.h
	@echo "Compiling $<"
	@$(CXX) $(Enclave_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

sgx_cosine_sim.so: Enclave/CosineEnclave_t.o $(Enclave_Cpp_Files:.cpp=.o)
	@echo "Linking enclave"
	@$(CXX) $^ -o $@ $(Enclave_Link_Flags)
	@echo "LINK =>  $@"

sgx_cosine_sim.signed.so: sgx_cosine_sim.so
	@echo "Signing enclave"
	@$(SGX_ENCLAVE_SIGNER) sign -key Enclave/Enclave_private.pem -enclave sgx_cosine_sim.so -out $@ -config Enclave/CosineEnclave.config.xml
	@echo "SIGN =>  $@"

clean:
	@rm -f sgx_cos_similarity sgx_cosine_sim.* App/CosineEnclave_u.* App/*.o Enclave/CosineEnclave_t.* Enclave/*.o
	@echo "Clean completed"