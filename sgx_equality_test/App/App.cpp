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

#define AES_KEY_SIZE 16  // 128 bits
#define AES_BLOCK_SIZE 16
#define MAX_FILE_SIZE (256 * 1024 * 1024)  // 256MB max file size to accommodate larger data

sgx_enclave_id_t global_eid = 0;

struct PerformanceMetrics {
    uint64_t total_entries;
    uint64_t total_exits;
    uint64_t total_time_us;
    uint64_t testing_time_us;
    uint64_t page_faults;
};

struct TimeStats {
    double min;  // Changed from uint64_t to double
    double mean;
    double median;
    double max;  // Changed from uint64_t to double
};


void cleanup_resources() {
    if (global_eid != 0) {
        sgx_status_t ret = ecall_cleanup(global_eid);
        if (ret != SGX_SUCCESS) {
            printf("Warning: Failed to cleanup enclave resources\n");
        }
        sgx_destroy_enclave(global_eid);
        global_eid = 0;
    }
}

bool initialize_encryption_key() {
    printf("\n=== Key Initialization ===\n");

    try {
        std::ifstream key_file("aes.key", std::ios::binary);
        if (!key_file) {
            throw std::system_error(errno, std::system_category(), "Failed to open AES key file");
        }

        // Use aligned buffer for key data
        alignas(16) std::vector<uint8_t> key_data(AES_KEY_SIZE + AES_BLOCK_SIZE);
        if (!key_file.read(reinterpret_cast<char*>(key_data.data()), key_data.size())) {
            throw std::runtime_error("Failed to read complete key data");
        }

        // Ensure proper alignment
        alignas(16) uint8_t aligned_key[AES_KEY_SIZE];
        alignas(16) uint8_t aligned_counter[AES_BLOCK_SIZE];

        memcpy(aligned_key, key_data.data(), AES_KEY_SIZE);
        memcpy(aligned_counter, key_data.data() + AES_KEY_SIZE, AES_BLOCK_SIZE);

        printf("AES Key (16 bytes): ");
        for (int i = 0; i < AES_KEY_SIZE; i++) {
            printf("%02X ", aligned_key[i]);
        }
        printf("\nAES Counter (16 bytes): ");
        for (int i = 0; i < AES_BLOCK_SIZE; i++) {
            printf("%02X ", aligned_counter[i]);
        }
        printf("\n");

        sgx_status_t ret_status;
        sgx_status_t status = ecall_initialize_aes_key(
            global_eid,
            &ret_status,
            aligned_key,
            AES_KEY_SIZE + AES_BLOCK_SIZE
        );

        if (status != SGX_SUCCESS || ret_status != SGX_SUCCESS) {
            throw std::runtime_error("Failed to initialize AES key in enclave");
        }

        printf("AES key initialized successfully in enclave\n");
        printf("=========================\n\n");
        return true;
    }
    catch (const std::exception& e) {
        printf("Error in key initialization: %s\n", e.what());
        return false;
    }
}

int initialize_enclave() {
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;

    try {
        ret = sgx_create_enclave(ENCLAVE_FILE, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
        if (ret != SGX_SUCCESS) {
            throw std::runtime_error("Enclave creation failed");
        }
        return 0;
    }
    catch (const std::exception& e) {
        printf("Error in enclave initialization: %s\n", e.what());
        return -1;
    }
}

bool load_sealed_data(const std::string& filename, std::vector<uint8_t>& sealed_data, bool is_test_data) {
    try {
        std::ifstream sealed_file(filename, std::ios::binary);
        if (!sealed_file) {
            throw std::system_error(errno, std::system_category(), 
                "Failed to open sealed data file: " + filename);
        }

        sealed_file.seekg(0, std::ios::end);
        size_t total_file_size = sealed_file.tellg();
        sealed_file.seekg(0, std::ios::beg);

        if (total_file_size > MAX_FILE_SIZE) {
            throw std::runtime_error("File size exceeds maximum allowed size");
        }

        printf("Total file size: %zu bytes\n", total_file_size);

        if (is_test_data) {
            if (total_file_size <= AES_BLOCK_SIZE) {
                throw std::runtime_error("Test file too small");
            }

            std::vector<uint8_t> counter(AES_BLOCK_SIZE);
            if (!sealed_file.read(reinterpret_cast<char*>(counter.data()), AES_BLOCK_SIZE)) {
                throw std::runtime_error("Failed to read counter data");
            }

            printf("Counter bytes: ");
            for (int i = 0; i < AES_BLOCK_SIZE; i++) {
                printf("%02X ", counter[i]);
            }
            printf("\n");

            sgx_status_t ret_status;
            sgx_status_t status = ecall_update_counter(
                global_eid, 
                &ret_status, 
                counter.data(), 
                AES_BLOCK_SIZE
            );

            if (status != SGX_SUCCESS || ret_status != SGX_SUCCESS) {
                throw std::runtime_error("Failed to update counter in enclave");
            }
        }

        size_t data_size = total_file_size - (is_test_data ? AES_BLOCK_SIZE : 0);
        sealed_data.clear();
        sealed_data.reserve(data_size);
        sealed_data.resize(data_size);

        if (!sealed_file.read(reinterpret_cast<char*>(sealed_data.data()), data_size)) {
            throw std::runtime_error("Failed to read sealed data");
        }

        printf("Data size: %zu bytes\n", data_size);
        printf("First 16 bytes of data: ");
        for (int i = 0; i < 16 && i < data_size; i++) {
            printf("%02X ", sealed_data[i]);
        }
        printf("\n");

        return true;
    }
    catch (const std::exception& e) {
        printf("Error loading sealed data: %s\n", e.what());
        return false;
    }
}

bool validate_sealed_data(const std::vector<uint8_t>& sealed_data, const char* type) {
    try {
        if (sealed_data.size() != sizeof(SecretData)) {
            throw std::runtime_error(std::string("Invalid ") + type + " data file size");
        }

        const auto* data = reinterpret_cast<const SecretData*>(sealed_data.data());
        if (data->version != CURRENT_VERSION) {
            throw std::runtime_error(std::string("Invalid ") + type + " data version");
        }

        if (data->count == 0 || data->count > MAX_VALUES) {
            throw std::runtime_error(std::string("Invalid ") + type + " data count");
        }

        return true;
    }
    catch (const std::exception& e) {
        printf("Validation error: %s\n", e.what());
        return false;
    }
}

TimeStats calculateTimeStats(const std::vector<double>& times) {
    if (times.empty()) {
        return {0.0, 0.0, 0.0, 0.0};  // Now all doubles
    }

    std::vector<double> sorted_times = times;
    std::sort(sorted_times.begin(), sorted_times.end());

    double min = sorted_times.front();
    double max = sorted_times.back();
    double mean = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

    double median;
    size_t size = sorted_times.size();
    if (size % 2 == 0) {
        median = (sorted_times[size/2 - 1] + sorted_times[size/2]) / 2.0;
    } else {
        median = sorted_times[size/2];
    }

    return {min, mean, median, max};  // All values are doubles now
}

PerformanceMetrics run_single_test(const std::string& secret_file, const std::string& test_file) {
    PerformanceMetrics metrics = {0, 0, 0, 0, 0};

    try {
        printf("Loading files: %s, %s\n", secret_file.c_str(), test_file.c_str());

        std::vector<uint8_t> secret_data;
        if (!load_sealed_data(secret_file, secret_data, false)) {
            throw std::runtime_error("Failed to load secret data");
        }

        auto total_start_time = std::chrono::high_resolution_clock::now();

        // Initialize secret data in enclave
        sgx_status_t init_ret_status;
        sgx_status_t init_status = ecall_initialize_secret_data(
            global_eid,
            &init_ret_status,
            secret_data.data(),
            secret_data.size()
        );

        if (init_status != SGX_SUCCESS || init_ret_status != SGX_SUCCESS) {
            throw std::runtime_error("Failed to initialize secret data in enclave");
        }

        // Load and decrypt test data
        std::vector<uint8_t> encrypted_test_data;
        if (!load_sealed_data(test_file, encrypted_test_data, true)) {
            throw std::runtime_error("Failed to load test data");
        }

        std::vector<uint8_t> decrypted_test_data(encrypted_test_data.size());

        // Decrypt test data
        sgx_status_t decrypt_ret_status;
        sgx_status_t decrypt_status = ecall_decrypt_test_data(
            global_eid,
            &decrypt_ret_status,
            encrypted_test_data.data(),
            encrypted_test_data.size(),
            decrypted_test_data.data(),
            decrypted_test_data.size()
        );

        if (decrypt_status != SGX_SUCCESS || decrypt_ret_status != SGX_SUCCESS) {
            throw std::runtime_error("Failed to decrypt test data");
        }

        const TestData* test_data = reinterpret_cast<const TestData*>(decrypted_test_data.data());
        if (test_data->version != CURRENT_VERSION || 
            test_data->count == 0 || 
            test_data->count > MAX_VALUES) {
            throw std::runtime_error("Invalid test data");
        }

        auto testing_start_time = std::chrono::high_resolution_clock::now();

        std::vector<std::pair<int, int>> results;
        metrics.total_entries = test_data->count;

        for (uint32_t i = 0; i < test_data->count; i++) {
            int check_result;
            sgx_status_t check_status = ecall_check_number(
                global_eid, 
                &check_result, 
                test_data->values[i]
            );

            if (check_status != SGX_SUCCESS) {
                continue;
            }
            results.push_back({test_data->values[i], check_result});
            metrics.total_exits++;
        }

        auto testing_end_time = std::chrono::high_resolution_clock::now();
        auto total_end_time = std::chrono::high_resolution_clock::now();

        metrics.testing_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            testing_end_time - testing_start_time).count();
        metrics.total_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            total_end_time - total_start_time).count();

        sgx_status_t pf_ret_status;
        sgx_status_t pf_status = ecall_get_page_fault_count(
            global_eid, 
            &pf_ret_status, 
            &metrics.page_faults
        );

        if (pf_status != SGX_SUCCESS || pf_ret_status != SGX_SUCCESS) {
            printf("Warning: Failed to get page fault count\n");
        }

        // Results summary
        int matches = 0, non_matches = 0, errors = 0;
        for (const auto& pair : results) {
            switch (pair.second) {
                case 1:  matches++; break;
                case 0:  non_matches++; break;
                default: errors++; break;
            }
        }

        printf("\nResults: %d matches, %d non-matches, %d errors\n", 
               matches, non_matches, errors);
        printf("Time: %lu µs, Avg: %.3f µs/op\n", 
               metrics.testing_time_us,
               static_cast<double>(metrics.testing_time_us) / results.size());

        return metrics;
    }
    catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return metrics;
    }
}

void ocall_print_string(const char* str) {
    printf("%s", str);
}

void ocall_print_error(const char* str) {
    printf("Error: %s\n", str);
}

int main(int argc, char* argv[]) {
    std::atexit(cleanup_resources);

    try {
        if (argc != 2) {
            throw std::runtime_error("Usage: " + std::string(argv[0]) + " <number_of_tests>");
        }

        int num_iterations = std::stoi(argv[1]);
        if (num_iterations <= 0) {
            throw std::runtime_error("Number of tests must be positive");
        }

        std::vector<PerformanceMetrics> all_metrics;

        if (initialize_enclave() < 0) {
            throw std::runtime_error("Enclave initialization failed");
        }

        if (!initialize_encryption_key()) {
            throw std::runtime_error("Failed to initialize encryption key");
        }

        printf("\nRunning test with %d different data files...\n", num_iterations);

        // Vector to store individual operation timings for detailed statistics
        std::vector<uint64_t> individual_operation_times;

        for (int i = 1; i <= num_iterations; i++) {
            printf("\nTest %d:\n", i);

            std::string secret_file = "tools/sealed_data/secret_numbers" + std::to_string(i) + ".dat";
            std::string test_file = "tools/sealed_data/test_numbers" + std::to_string(i) + ".dat";

            auto test_start = std::chrono::high_resolution_clock::now();
            PerformanceMetrics metrics = run_single_test(secret_file, test_file);
            auto test_end = std::chrono::high_resolution_clock::now();

            // Calculate and store individual test duration
            uint64_t test_duration = std::chrono::duration_cast<std::chrono::microseconds>(
                test_end - test_start).count();

            all_metrics.push_back(metrics);

            // Detailed test statistics
            printf("\nDetailed Test %d Statistics:\n", i);
            printf("-------------------------\n");
            printf("Total entries processed: %lu\n", metrics.total_entries);
            printf("Successful operations: %lu\n", metrics.total_exits);
            printf("Total test time: %lu µs\n", test_duration);
            printf("Pure processing time: %lu µs\n", metrics.testing_time_us);
            printf("Overhead time: %lu µs\n", test_duration - metrics.testing_time_us);
            printf("Page faults: %lu\n", metrics.page_faults);

            if (metrics.total_exits > 0) {
                double avg_time = static_cast<double>(metrics.testing_time_us) / metrics.total_exits;
                printf("Average time per operation: %.3f µs\n", avg_time);
                printf("Operations per second: %.2f\n", 
                       1000000.0 * metrics.total_exits / metrics.testing_time_us);
            }
        }

        // Calculate and print comprehensive statistics
        printf("\nComprehensive Performance Analysis:\n");
        printf("=================================\n");

        // Calculate aggregate statistics
        uint64_t total_entries = 0;
        uint64_t total_exits = 0;
        uint64_t total_page_faults = 0;
        uint64_t total_time_us = 0;
        uint64_t total_testing_time_us = 0;

        for (const auto& metrics : all_metrics) {
            total_entries += metrics.total_entries;
            total_exits += metrics.total_exits;
            total_page_faults += metrics.page_faults;
            total_time_us += metrics.total_time_us;
            total_testing_time_us += metrics.testing_time_us;
        }

        // Calculate timing statistics
        std::vector<uint64_t> execution_times;
        std::vector<uint64_t> testing_times;
        std::vector<double> operation_times;

        for (const auto& metrics : all_metrics) {
            execution_times.push_back(metrics.total_time_us);
            testing_times.push_back(metrics.testing_time_us);
            if (metrics.total_exits > 0) {
                operation_times.push_back(
                    static_cast<double>(metrics.testing_time_us) / metrics.total_exits);
            }
        }

        std::sort(execution_times.begin(), execution_times.end());
        std::sort(testing_times.begin(), testing_times.end());
        std::sort(operation_times.begin(), operation_times.end());

        // Print aggregate statistics
        printf("\nAggregate Statistics:\n");
        printf("Total entries processed: %lu\n", total_entries);
        printf("Total successful operations: %lu\n", total_exits);
        printf("Total page faults: %lu\n", total_page_faults);
        printf("Total execution time: %lu µs\n", total_time_us);
        printf("Total testing time: %lu µs\n", total_testing_time_us);
        printf("Average page faults per test: %.2f\n", 
               static_cast<double>(total_page_faults) / num_iterations);

        // Print timing statistics
        printf("\nExecution Time Statistics (µs):\n");
        printf("Minimum: %lu\n", execution_times.front());
        printf("Maximum: %lu\n", execution_times.back());
        printf("Mean: %.2f\n", 
               std::accumulate(execution_times.begin(), execution_times.end(), 0.0) / execution_times.size());
        printf("Median: %lu\n", execution_times[execution_times.size() / 2]);

        printf("\nTesting Time Statistics (µs):\n");
        printf("Minimum: %lu\n", testing_times.front());
        printf("Maximum: %lu\n", testing_times.back());
        printf("Mean: %.2f\n", 
               std::accumulate(testing_times.begin(), testing_times.end(), 0.0) / testing_times.size());
        printf("Median: %lu\n", testing_times[testing_times.size() / 2]);

        if (!operation_times.empty()) {
            printf("\nPer-Operation Time Statistics (µs):\n");
            printf("Minimum: %.3f\n", operation_times.front());
            printf("Maximum: %.3f\n", operation_times.back());
            printf("Mean: %.3f\n", 
                   std::accumulate(operation_times.begin(), operation_times.end(), 0.0) / operation_times.size());
            printf("Median: %.3f\n", operation_times[operation_times.size() / 2]);
        }

        // Performance metrics
        printf("\nPerformance Metrics:\n");
        printf("Average operations per second: %.2f\n", 
               1000000.0 * total_exits / total_testing_time_us);
        printf("Average time per operation: %.3f µs\n", 
               static_cast<double>(total_testing_time_us) / total_exits);
        printf("Processing efficiency: %.2f%%\n", 
               100.0 * total_testing_time_us / total_time_us);

        return 0;
    }
    catch (const std::exception& e) {
        printf("Fatal error: %s\n", e.what());
        return 1;
    }
}
