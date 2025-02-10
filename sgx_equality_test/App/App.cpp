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

sgx_enclave_id_t global_eid = 0;

// Performance tracking structure
struct PerformanceMetrics {
    uint64_t total_entries;
    uint64_t total_exits;
    double total_time_ms;
    uint64_t page_faults;
};

// Statistics structure
struct TimeStats {
    double min;
    double mean;
    double median;
    double max;
};

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

TimeStats calculateTimeStats(const std::vector<double>& times) {
    if (times.empty()) {
        return {0.0, 0.0, 0.0, 0.0};
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

    return {min, mean, median, max};
}

PerformanceMetrics run_single_test(const std::string& secret_file, const std::string& test_file) {
    PerformanceMetrics metrics = {0, 0, 0.0, 0};

    // Load sealed secret data
    std::vector<uint8_t> sealed_data;
    if (!load_sealed_data(secret_file, sealed_data) ||
        !validate_sealed_data(sealed_data, "secret")) {
        return metrics;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

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
        return metrics;
    }

    // Load sealed test data
    std::vector<uint8_t> sealed_test_data;
    if (!load_sealed_data(test_file, sealed_test_data) ||
        !validate_sealed_data(sealed_test_data, "test")) {
        return metrics;
    }

    const TestData* test_data = reinterpret_cast<const TestData*>(sealed_test_data.data());

    // Process each test number
    int result;
    std::vector<std::pair<int, int>> results;

    metrics.total_entries = test_data->count;

    for (uint32_t i = 0; i < test_data->count; i++) {
        status = ecall_check_number(global_eid, &result, test_data->values[i]);
        if (status != SGX_SUCCESS) {
            printf("Enclave call failed for number %d\n", test_data->values[i]);
            continue;
        }
        results.push_back({test_data->values[i], result});
        metrics.total_exits++;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    metrics.total_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    // Get page fault count from enclave
    status = ecall_get_page_fault_count(global_eid, &ret_status, &metrics.page_faults);
    if (status != SGX_SUCCESS || ret_status != SGX_SUCCESS) {
        printf("Failed to get page fault count\n");
    }

    // Print results
    for (const auto& pair : results) {
        if (pair.second == 1) {
            printf("Number %d is in the secret list\n", pair.first);
        } else if (pair.second == 0) {
            printf("Number %d is not in the secret list\n", pair.first);
        } else if (pair.second == -2) {
            printf("Error: Empty secret data for number %d\n", pair.first);
        } else {
            printf("Error: Secret data not initialized for number %d\n", pair.first);
        }
    }

    return metrics;
}

int main(int argc, char* argv[])
{
    const int NUM_ITERATIONS = 25;
    std::vector<PerformanceMetrics> all_metrics;
    std::vector<double> execution_times;

    if (initialize_enclave() < 0) {
        printf("Enclave initialization failed.\n");
        return -1;
    }

    printf("\nRunning test with %d different data files...\n", NUM_ITERATIONS);

    // Run tests with different data files
    for (int i = 1; i <= NUM_ITERATIONS; i++) {
        printf("\nTest %d:\n", i);

        std::string secret_file = "tools/sealed_data/secret_numbers" + std::to_string(i) + ".dat";
        std::string test_file = "tools/sealed_data/test_numbers" + std::to_string(i) + ".dat";

        PerformanceMetrics metrics = run_single_test(secret_file, test_file);
        all_metrics.push_back(metrics);
        execution_times.push_back(metrics.total_time_ms);
    }

    // Print performance statistics
    printf("\nPerformance Statistics:\n");
    printf("----------------------\n");
    for (size_t i = 0; i < all_metrics.size(); i++) {
        const auto& metrics = all_metrics[i];
        printf("\nTest %zu:\n", i + 1);
        printf("Total EENTER calls: %lu\n", metrics.total_entries);
        printf("Total EEXIT calls: %lu\n", metrics.total_exits);
        printf("Total execution time: %.2f ms\n", metrics.total_time_ms);
        printf("Page faults: %lu\n", metrics.page_faults);
        printf("Average time per operation: %.3f ms\n", 
               metrics.total_time_ms / metrics.total_entries);
    }

    // Calculate and display time statistics
    TimeStats stats = calculateTimeStats(execution_times);
    printf("\nExecution Time Statistics (ms):\n");
    printf("------------------------------\n");
    printf("Minimum: %.2f\n", stats.min);
    printf("Mean: %.2f\n", stats.mean);
    printf("Median: %.2f\n", stats.median);
    printf("Maximum: %.2f\n", stats.max);

    // Print all execution times in array format
    printf("\nExecution Times: [");
    for (size_t i = 0; i < execution_times.size(); i++) {
        printf("%.2f%s", execution_times[i], 
            (i < execution_times.size() - 1) ? ", " : "");
    }
    printf("]\n");

    sgx_destroy_enclave(global_eid);
    return 0;

}
