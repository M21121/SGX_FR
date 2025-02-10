// App.h
#ifndef _APP_H_
#define _APP_H_

#include <stdio.h>
#include <string>
#include <vector>
#include "sgx_urts.h"

#define ENCLAVE_FILE "sgx_equality_test.signed.so"

// Function declarations
bool load_sealed_data(const std::string& filename, std::vector<uint8_t>& sealed_data);
bool validate_sealed_data(const std::vector<uint8_t>& sealed_data, const char* type);

#endif
