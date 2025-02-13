######## SGX SDK Settings ########
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
SGX_DEBUG ?= 1

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_FLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_FLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
	SGX_COMMON_FLAGS += -O0 -g
else
	SGX_COMMON_FLAGS += -O2
endif

######## App Settings ########

App_Cpp_Files := App/App.cpp
App_C_Files := App/Enclave_u.c
App_Include_Paths := -IApp -I$(SGX_SDK)/include -Icommon

App_C_Flags := -fPIC -Wno-attributes $(App_Include_Paths)
App_Cpp_Flags := $(App_C_Flags) -std=c++11

App_Link_Flags := -L$(SGX_LIBRARY_PATH) -lsgx_urts -lpthread -lssl -lcrypto

App_C_Objects := $(App_C_Files:.c=.o)
App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)

App_Name := sgx_equality_test

######## Enclave Settings ########

ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif
Crypto_Library_Name := sgx_tcrypto

Enclave_Cpp_Files := Enclave/Enclave.cpp
Enclave_Include_Paths := -I$(SGX_SDK)/include \
						-I$(SGX_SDK)/include/tlibc \
						-I$(SGX_SDK)/include/libcxx \
						-I../common

Enclave_C_Flags := $(SGX_COMMON_FLAGS) -nostdinc -fvisibility=hidden -fpie -ffunction-sections -fdata-sections $(Enclave_Include_Paths)
Enclave_Cpp_Flags := $(Enclave_C_Flags) -std=c++11 -nostdinc++

Enclave_Link_Flags := $(SGX_COMMON_FLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tcxx -l$(Crypto_Library_Name) -l$(Service_Library_Name) -lsgx_tcrypto -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic \
	-Wl,--defsym,__ImageBase=0 -Wl,--gc-sections

Enclave_Cpp_Objects := $(Enclave_Cpp_Files:.cpp=.o)

Enclave_Name := sgx_equality_test.so
Signed_Enclave_Name := sgx_equality_test.signed.so
Enclave_Config_File := Enclave/Enclave.config.xml

######## Rules ########

.PHONY: all clean

all: $(App_Name) $(Signed_Enclave_Name)

######## App Objects ########

App/Enclave_u.c: $(SGX_EDGER8R) Enclave/Enclave.edl
	@cd App && $(SGX_EDGER8R) --untrusted ../Enclave/Enclave.edl --search-path ../Enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

App/Enclave_u.o: App/Enclave_u.c
	@$(CC) $(App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

App/%.o: App/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(App_Name): App/Enclave_u.o $(App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(App_Link_Flags)
	@echo "LINK =>  $@"

######## Enclave Objects ########

Enclave/Enclave_t.c: $(SGX_EDGER8R) Enclave/Enclave.edl
	@cd Enclave && $(SGX_EDGER8R) --trusted ../Enclave/Enclave.edl --search-path ../Enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

Enclave/Enclave_t.o: Enclave/Enclave_t.c
	@$(CC) $(Enclave_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

Enclave/%.o: Enclave/%.cpp
	@$(CXX) $(Enclave_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(Enclave_Name): Enclave/Enclave_t.o $(Enclave_Cpp_Objects)
	@$(CXX) $^ -o $@ $(Enclave_Link_Flags)
	@echo "LINK =>  $@"

$(Signed_Enclave_Name): $(Enclave_Name)
	@$(SGX_ENCLAVE_SIGNER) sign -key Enclave/Enclave_private.pem -enclave $(Enclave_Name) -out $@ -config $(Enclave_Config_File)
	@echo "SIGN =>  $@"

clean:
	@rm -f $(App_Name) $(Enclave_Name) $(Signed_Enclave_Name) App/Enclave_u.* Enclave/Enclave_t.* *.o App/*.o Enclave/*.o
