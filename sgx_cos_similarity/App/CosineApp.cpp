// ./App/CosineApp.cpp
#include "CosineApp.h"
#include "CosineEnclave_u.h"
#include "shared_types.h"
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <unistd.h> 
#include <cstdio>    
#include <limits.h>
#include <array>  

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
    std::array<char, 1024> cwd;
    if (getcwd(cwd.data(), cwd.size()) != nullptr) {
        printf("Current working directory: %s\n", cwd.data());
    }

    std::ifstream sealed_file(filename, std::ios::binary);
    if (!sealed_file) {
        printf("Failed to open sealed data file: %s\n", filename.c_str());
        return false;
    }

    sealed_file.seekg(0, std::ios::end);
    size_t file_size = sealed_file.tellg();
    printf("File size of %s: %zu bytes\n", filename.c_str(), file_size);
    sealed_data.resize(file_size);
    sealed_file.seekg(0, std::ios::beg);
    sealed_file.read(reinterpret_cast<char*>(sealed_data.data()), sealed_data.size());
    sealed_file.close();
    return true;
}

bool validate_reference_data(const std::vector<uint8_t>& sealed_data) {
    printf("Validating reference data of size: %zu bytes\n", sealed_data.size());

    // Check minimum size for header (version + count)
    if (sealed_data.size() < sizeof(uint32_t) * 2) {
        printf("Invalid reference vectors file size. Too small for header.\n");
        return false;
    }

    // Read version and count
    uint32_t version, count;
    memcpy(&version, sealed_data.data(), sizeof(uint32_t));
    memcpy(&count, sealed_data.data() + sizeof(uint32_t), sizeof(uint32_t));

    // Calculate expected size
    size_t expected_size = (sizeof(uint32_t) * 2) + (count * sizeof(vector_t));

    printf("Reference data version: %u\n", version);
    printf("Reference vector count: %u\n", count);
    printf("Expected size: %zu, Got: %zu\n", expected_size, sealed_data.size());

    if (version != CURRENT_VERSION) {
        printf("Invalid reference data version. Expected: %d, Got: %u\n", 
               CURRENT_VERSION, version);
        return false;
    }

    if (count == 0 || count > MAX_VECTORS) {
        printf("Invalid reference vector count. Must be between 1 and %d\n", MAX_VECTORS);
        return false;
    }

    if (sealed_data.size() != expected_size) {
        printf("Invalid reference vectors file size. Expected: %zu, Got: %zu\n", 
               expected_size, sealed_data.size());
        return false;
    }

    return true;
}

bool validate_query_data(const std::vector<uint8_t>& sealed_data) {
    if (sealed_data.size() != sizeof(QueryVector)) {
        printf("Invalid query vector file size. Expected: %zu, Got: %zu\n",
               sizeof(QueryVector), sealed_data.size());
        return false;
    }

    const auto* data = reinterpret_cast<const QueryVector*>(sealed_data.data());
    if (data->version != CURRENT_VERSION) {
        printf("Invalid query data version. Expected: %d, Got: %u\n",
               CURRENT_VERSION, data->version);
        return false;
    }

    if (data->count != 1) {
        printf("Invalid query vector count. Expected: 1, Got: %u\n", data->count);
        return false;
    }

    return true;
}

int main(void)
{
    if (initialize_enclave() < 0) {
        printf("Enclave initialization failed.\n");
        return -1;
    }

    // Load sealed reference vectors
    std::vector<uint8_t> sealed_data;
    if (!load_sealed_data("tools/sealed_data/reference_vectors.dat", sealed_data) ||
        !validate_reference_data(sealed_data)) {
        sgx_destroy_enclave(global_eid);
        return -1;
    }

    // Initialize reference vectors in enclave
    sgx_status_t ret_status;
    printf("Attempting to initialize enclave with %zu bytes of sealed data\n", sealed_data.size());

    sgx_status_t status = ecall_initialize_reference_vectors(
        global_eid,
        &ret_status,
        sealed_data.data(),
        sealed_data.size()
    );

    const char* error_message;
    switch (status) {
        case SGX_SUCCESS:
            error_message = "Success";
            break;
        case SGX_ERROR_INVALID_PARAMETER:
            error_message = "Invalid parameter";
            break;
        case SGX_ERROR_OUT_OF_MEMORY:
            error_message = "Out of memory";
            break;
        default:
            error_message = "Unknown error";
    }

    printf("Initialization status: %d (0x%x) - %s, Return status: %d (0x%x)\n", 
        status, status, error_message, ret_status, ret_status);

    if (status != SGX_SUCCESS || ret_status != SGX_SUCCESS) {
        printf("Failed to initialize reference vectors in enclave\n");
        sgx_destroy_enclave(global_eid);
        return -1;
    }

    // Load sealed query vector
    std::vector<uint8_t> sealed_query_data;
    if (!load_sealed_data("tools/sealed_data/query_vector.dat", sealed_query_data) ||
        !validate_query_data(sealed_query_data)) {
        sgx_destroy_enclave(global_eid);
        return -1;
    }

    const QueryVector* query_data = reinterpret_cast<const QueryVector*>(sealed_query_data.data());

    // Compute cosine similarity
    float similarity;
    status = ecall_compute_cosine_similarity(
        global_eid,
        &similarity,
        query_data->vector.data()
    );

    if (status != SGX_SUCCESS) {
        printf("Enclave call failed with status: %d\n", status);
        sgx_destroy_enclave(global_eid);
        return -1;
    }

    if (similarity == -2.0f) {
        printf("Error: Invalid computation or uninitialized data\n");
    } else if (similarity == -1.0f) {
        printf("Error: Zero magnitude vector detected\n");
    } else if (similarity >= -1.0f && similarity <= 1.0f) {
        printf("Maximum cosine similarity: %0.6f\n", similarity);
    } else {
        printf("Error: Invalid similarity value: %f\n", similarity);
    }

    // Cleanup
    ecall_cleanup_reference_vectors(global_eid);
    sgx_destroy_enclave(global_eid);
    return 0;
}
