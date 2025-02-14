// App.cpp
#include "App.h"
#include "Enclave_u.h"
#include "shared_types.h"
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <sgx_urts.h>
#include <sgx_eid.h>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <unistd.h>  
#include <stdarg.h>
#include <stdio.h>
#include <memory>
#include <system_error>
#include <sgx_tcrypto.h>

#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16
#define MAX_FILE_SIZE (256 * 1024 * 1024)

sgx_enclave_id_t global_eid = 0;

struct TestResults {
    int matches;
    int non_matches;
    int errors;
    uint64_t total_time_us;
    uint64_t processing_time_us;
    uint64_t encryption_time_us;
    uint64_t page_faults;
};

void cleanup_resources() {
    if (global_eid != 0) {
        sgx_status_t ret = ecall_cleanup(global_eid);
        if (ret != SGX_SUCCESS) {
            printf("Failed to cleanup enclave resources\n");
        }
        sgx_destroy_enclave(global_eid);
        global_eid = 0;
    }
}

bool initialize_encryption_key() {
    try {
        std::ifstream key_file("aes.key", std::ios::binary);
        if (!key_file) {
            return false;
        }

        alignas(16) std::vector<uint8_t> key_data(AES_KEY_SIZE + AES_BLOCK_SIZE);
        if (!key_file.read(reinterpret_cast<char*>(key_data.data()), key_data.size())) {
            return false;
        }

        alignas(16) uint8_t aligned_key[AES_KEY_SIZE];
        alignas(16) uint8_t aligned_counter[AES_BLOCK_SIZE];
        memcpy(aligned_key, key_data.data(), AES_KEY_SIZE);
        memcpy(aligned_counter, key_data.data() + AES_KEY_SIZE, AES_BLOCK_SIZE);

        sgx_status_t ret_status;
        sgx_status_t status = ecall_initialize_aes_key(
            global_eid,
            &ret_status,
            aligned_key,
            AES_KEY_SIZE + AES_BLOCK_SIZE
        );

        return (status == SGX_SUCCESS && ret_status == SGX_SUCCESS);
    }
    catch (...) {
        return false;
    }
}

int initialize_enclave() {
    sgx_status_t ret = sgx_create_enclave(ENCLAVE_FILE, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    return (ret == SGX_SUCCESS) ? 0 : -1;
}

bool load_sealed_data(const std::string& filename, std::vector<uint8_t>& sealed_data, bool is_test_data) {
    try {
        std::ifstream sealed_file(filename, std::ios::binary);
        if (!sealed_file) {
            return false;
        }

        sealed_file.seekg(0, std::ios::end);
        size_t total_file_size = sealed_file.tellg();
        sealed_file.seekg(0, std::ios::beg);

        if (total_file_size > MAX_FILE_SIZE) {
            return false;
        }

        if (is_test_data) {
            std::vector<uint8_t> counter(AES_BLOCK_SIZE);
            if (!sealed_file.read(reinterpret_cast<char*>(counter.data()), AES_BLOCK_SIZE)) {
                return false;
            }

            sgx_status_t ret_status;
            if (ecall_update_counter(global_eid, &ret_status, counter.data(), AES_BLOCK_SIZE) != SGX_SUCCESS) {
                return false;
            }
        }

        size_t data_size = total_file_size - (is_test_data ? AES_BLOCK_SIZE : 0);
        sealed_data.resize(data_size);

        if (!sealed_file.read(reinterpret_cast<char*>(sealed_data.data()), data_size)) {
            return false;
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

bool validate_sealed_data(const std::vector<uint8_t>& sealed_data, const char* type) {
    if (sealed_data.size() != sizeof(SecretData)) {
        return false;
    }

    const auto* data = reinterpret_cast<const SecretData*>(sealed_data.data());
    return (data->version == CURRENT_VERSION && data->count > 0 && data->count <= MAX_VALUES);
}

TestResults run_test_iteration(const std::string& secret_file, const std::string& test_file) {
    TestResults results = {0, 0, 0, 0, 0, 0, 0};  // Initialize with 0 for all fields
    auto total_start = std::chrono::high_resolution_clock::now();

    std::vector<uint8_t> secret_data;
    if (!load_sealed_data(secret_file, secret_data, false)) {
        return results;
    }

    if (!validate_sealed_data(secret_data, "secret")) {
        return results;
    }

    sgx_status_t init_ret_status;
    if (ecall_initialize_secret_data(global_eid, &init_ret_status, 
        secret_data.data(), secret_data.size()) != SGX_SUCCESS) {
        return results;
    }

    std::vector<uint8_t> encrypted_test_data;
    if (!load_sealed_data(test_file, encrypted_test_data, true)) {
        return results;
    }

    std::vector<uint8_t> decrypted_test_data(encrypted_test_data.size());
    sgx_status_t decrypt_ret_status;

    auto processing_start = std::chrono::high_resolution_clock::now();

    if (ecall_decrypt_test_data(global_eid, &decrypt_ret_status,
        encrypted_test_data.data(), encrypted_test_data.size(),
        decrypted_test_data.data(), decrypted_test_data.size()) != SGX_SUCCESS) {
        return results;
    }

    const TestData* test_data = reinterpret_cast<const TestData*>(decrypted_test_data.data());
    if (test_data->version != CURRENT_VERSION || test_data->count == 0 || test_data->count > MAX_VALUES) {
        return results;
    }

    // Process each test value and count results
    for (uint32_t i = 0; i < test_data->count; i++) {
        alignas(16) uint8_t encrypted_result[AES_BLOCK_SIZE];
        sgx_status_t check_ret_status;

        // Measure encryption time from outside the enclave
        auto encryption_start = std::chrono::high_resolution_clock::now();

        if (ecall_check_number_encrypted(global_eid, &check_ret_status,
            test_data->values[i], encrypted_result, AES_BLOCK_SIZE) != SGX_SUCCESS) {
            results.errors++;
            continue;
        }

        auto encryption_end = std::chrono::high_resolution_clock::now();
        uint64_t encryption_duration = std::chrono::duration_cast<std::chrono::microseconds>(
            encryption_end - encryption_start).count();
        results.encryption_time_us += encryption_duration;

        // Decrypt the result
        alignas(16) uint8_t decrypted_result;
        std::ifstream key_file("aes.key", std::ios::binary);
        if (!key_file) {
            results.errors++;
            continue;
        }

        std::vector<uint8_t> key_data(AES_KEY_SIZE + AES_BLOCK_SIZE);
        if (!key_file.read(reinterpret_cast<char*>(key_data.data()), key_data.size())) {
            results.errors++;
            continue;
        }

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            results.errors++;
            continue;
        }

        if (EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key_data.data(), 
            key_data.data() + AES_KEY_SIZE) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            results.errors++;
            continue;
        }

        int len;
        if (EVP_DecryptUpdate(ctx, &decrypted_result, &len, encrypted_result, 1) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            results.errors++;
            continue;
        }

        int final_len;
        if (EVP_DecryptFinal_ex(ctx, &decrypted_result + len, &final_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            results.errors++;
            continue;
        }

        EVP_CIPHER_CTX_free(ctx);

        // Count matches/non-matches
        if (decrypted_result == 1) {
            results.matches++;
        } else {
            results.non_matches++;
        }
    }

    auto processing_end = std::chrono::high_resolution_clock::now();
    auto total_end = std::chrono::high_resolution_clock::now();

    results.processing_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        processing_end - processing_start).count();
    results.total_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        total_end - total_start).count();

    // Get page fault count
    sgx_status_t pf_ret_status;
    ecall_get_page_fault_count(global_eid, &pf_ret_status, &results.page_faults);

    return results;
}

void ocall_print_string(const char* str) {
    printf("%s", str);
}

void ocall_print_error(const char* str) {
    printf("Error: %s\n", str);
}

int main(int argc, char* argv[]) {
    std::atexit(cleanup_resources);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_tests>\n", argv[0]);
        return 1;
    }

    int num_iterations;
    try {
        num_iterations = std::stoi(argv[1]);
        if (num_iterations <= 0) {
            throw std::invalid_argument("Number of tests must be positive");
        }
    }
    catch (const std::exception&) {
        fprintf(stderr, "Invalid number of tests specified\n");
        return 1;
    }

    if (initialize_enclave() < 0) {
        fprintf(stderr, "Enclave initialization failed\n");
        return 1;
    }

    if (!initialize_encryption_key()) {
        fprintf(stderr, "Failed to initialize encryption key\n");
        return 1;
    }

    // Print CSV header with new encryption time column
    printf("Test,Matches,NonMatches,Errors,TotalTime_us,ProcessingTime_us,EncryptionTime_us,OverheadTime_us,PageFaults\n");

    for (int i = 1; i <= num_iterations; i++) {
        std::string secret_file = "tools/sealed_data/secret_numbers" + std::to_string(i) + ".dat";
        std::string test_file = "tools/sealed_data/test_numbers" + std::to_string(i) + ".dat";

        TestResults results = run_test_iteration(secret_file, test_file);

        // Print CSV formatted line with encryption time
        printf("%d,%d,%d,%d,%lu,%lu,%lu,%lu,%lu\n",
               i,
               results.matches,
               results.non_matches,
               results.errors,
               results.total_time_us,
               results.processing_time_us,
               results.encryption_time_us,
               results.total_time_us - results.processing_time_us,
               results.page_faults);
    }

    return 0;
}


