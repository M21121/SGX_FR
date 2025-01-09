// App.cpp
#include "App.h"
#include "Enclave_u.h"
#include "shared_types.h"
#include <string>
#include <vector>
#include <fstream>
#include "shared_types.h" 

sgx_enclave_id_t global_eid = 0;

// Initialize the enclave
int initialize_enclave(void)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    ret = sgx_create_enclave(ENCLAVE_FILE, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        printf("Enclave creation failed\n");
        return -1;
    }
    return 0;
}

bool load_sealed_data(const std::string& filename, std::vector<uint8_t>& sealed_data) {
    std::ifstream sealed_file(filename, std::ios::binary);
    if (!sealed_file) {
        printf("Failed to open sealed data file: %s\n", filename.c_str());
        return false;
    }

    sealed_file.seekg(0, std::ios::end);
    sealed_data.resize(sealed_file.tellg());
    sealed_file.seekg(0, std::ios::beg);
    sealed_file.read(reinterpret_cast<char*>(sealed_data.data()), sealed_data.size());
    sealed_file.close();
    return true;
}

bool validate_sealed_data(const std::vector<uint8_t>& sealed_data, const char* type) {
    if (sealed_data.size() != sizeof(SecretData)) {
        printf("Invalid %s data file size\n", type);
        return false;
    }

    const auto* data = reinterpret_cast<const SecretData*>(sealed_data.data());
    if (data->version != CURRENT_VERSION) {
        printf("Invalid %s data version\n", type);
        return false;
    }

    if (data->count == 0 || data->count > MAX_VALUES) {
        printf("Invalid %s data count\n", type);
        return false;
    }

    return true;
}

int main(int argc, char* argv[])
{
    if (initialize_enclave() < 0) {
        printf("Enclave initialization failed.\n");
        return -1;
    }

    // Load sealed secret data
    std::vector<uint8_t> sealed_data;
    if (!load_sealed_data("tools/sealed_data/secret_numbers.dat", sealed_data) ||
        !validate_sealed_data(sealed_data, "secret")) {
        sgx_destroy_enclave(global_eid);
        return -1;
    }

    // Initialize secret data in enclave
    sgx_status_t ret_status;
    sgx_status_t status = ecall_initialize_secret_data(
        global_eid,
        &ret_status,
        sealed_data.data(),
        sealed_data.size()
    );

    if (status != SGX_SUCCESS || ret_status != SGX_SUCCESS) {
        printf("Failed to initialize secret data in enclave\n");
        sgx_destroy_enclave(global_eid);
        return -1;
    }

    // Load sealed test data
    std::vector<uint8_t> sealed_test_data;
    if (!load_sealed_data("tools/sealed_data/test_numbers.dat", sealed_test_data) ||
        !validate_sealed_data(sealed_test_data, "test")) {
        sgx_destroy_enclave(global_eid);
        return -1;
    }

    const TestData* test_data = reinterpret_cast<const TestData*>(sealed_test_data.data());

    // Process each test number
    int result;
    for (uint32_t i = 0; i < test_data->count; i++) {
        status = ecall_check_number(global_eid, &result, test_data->values[i]);
        if (status != SGX_SUCCESS) {
            printf("Enclave call failed for number %d\n", test_data->values[i]);
            continue;
        }

        if (result == 1) {
            printf("Number %d is in the secret list\n", test_data->values[i]);
        } else if (result == 0) {
            printf("Number %d is not in the secret list\n", test_data->values[i]);
        } else {
            printf("Error: Secret data not initialized\n");
        }
    }

    sgx_destroy_enclave(global_eid);
    return 0;
}
