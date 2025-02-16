// Enclave.cpp
#include "Enclave_t.h"
#include "../common/shared_types.h"
#include <sgx_trts.h>
#include <sgx_tseal.h>
#include <string.h>
#include <sgx_tcrypto.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>

static SecretData* g_secret_data = nullptr;
static bool g_is_initialized = false;
static uint64_t g_page_fault_count = 0;

// AES key and counter definitions
#define AES_KEY_SIZE 16  // 128 bits
#define AES_BLOCK_SIZE 16
#define CHUNK_SIZE 8192  // Process data in 8KB chunks for better memory management

static uint8_t g_aes_key[AES_KEY_SIZE];
static uint8_t g_aes_counter[AES_BLOCK_SIZE];
static bool g_aes_initialized = false;

struct TimingInfo {
    uint64_t start_time;
    uint64_t encryption_time;
    uint64_t decryption_time;
    uint64_t processing_time;
    uint64_t total_time;
};

static TimingInfo g_timing = {0, 0, 0, 0, 0};

// Helper function to get minimum of two values
static inline size_t min_size_t(size_t a, size_t b) {
    return (a < b) ? a : b;
}

// Helper function for aligned memory allocation
static void* aligned_malloc(size_t size, size_t alignment) {
    void* ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return ptr;
}

// Helper function to format and print messages
void print_debug(const char* format, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    ocall_print_string(buf);
}

// Helper function to allocate secret data with proper alignment
static bool allocate_secret_data() {
    size_t required_size = sizeof(SecretData);
    void* ptr = aligned_malloc(required_size, 16);  // 16-byte alignment for better performance

    if (!ptr) {
        return false;
    }

    g_secret_data = (SecretData*)ptr;
    return true;
}

sgx_status_t ecall_initialize_aes_key(const uint8_t* key_data, size_t key_size) {
    if (!key_data || key_size != (AES_KEY_SIZE + AES_BLOCK_SIZE)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // Ensure proper alignment of key data
    memcpy(g_aes_key, key_data, AES_KEY_SIZE);
    memcpy(g_aes_counter, key_data + AES_KEY_SIZE, AES_BLOCK_SIZE);
    g_aes_initialized = true;

    return SGX_SUCCESS;
}

sgx_status_t ecall_initialize_secret_data(const uint8_t* sealed_data, size_t sealed_size) {
    if (!sealed_data || sealed_size != sizeof(SecretData)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // Clean up any existing data
    if (g_secret_data) {
        memset(g_secret_data, 0, sizeof(SecretData));  // Secure cleanup
        free(g_secret_data);
        g_secret_data = nullptr;
    }

    // Allocate memory for the secret data
    if (!allocate_secret_data()) {
        return SGX_ERROR_OUT_OF_MEMORY;
    }

    // Copy the sealed data in chunks to prevent memory pressure
    size_t remaining = sealed_size;
    size_t offset = 0;
    const uint8_t* src = sealed_data;
    uint8_t* dst = (uint8_t*)g_secret_data;

    while (remaining > 0) {
        size_t chunk = min_size_t(remaining, CHUNK_SIZE);
        memcpy(dst + offset, src + offset, chunk);
        remaining -= chunk;
        offset += chunk;
    }

    // Validate the data
    if (g_secret_data->version != CURRENT_VERSION) {
        memset(g_secret_data, 0, sizeof(SecretData));
        free(g_secret_data);
        g_secret_data = nullptr;
        return SGX_ERROR_INVALID_VERSION;
    }

    if (g_secret_data->count == 0 || g_secret_data->count > MAX_VALUES) {
        memset(g_secret_data, 0, sizeof(SecretData));
        free(g_secret_data);
        g_secret_data = nullptr;
        return SGX_ERROR_UNEXPECTED;
    }

    g_is_initialized = true;
    g_page_fault_count = 0;  // Reset page fault counter

    return SGX_SUCCESS;
}

sgx_status_t ecall_decrypt_test_data(const uint8_t* encrypted_data, size_t encrypted_size,
                                    uint8_t* decrypted_data, size_t decrypted_size) {
    if (!encrypted_data || !decrypted_data || 
        encrypted_size == 0 || encrypted_size != decrypted_size || 
        encrypted_size != sizeof(TestData) || !g_aes_initialized) {
        print_debug("Error: Invalid parameters for decryption\n");
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // Start timing decryption
    uint64_t decrypt_start;
    uint64_t retval;
    if (ocall_get_current_time(&retval, &decrypt_start) != SGX_SUCCESS) {
        return SGX_ERROR_UNEXPECTED;
    }

    // Ensure 16-byte alignment
    alignas(16) uint8_t aligned_key[AES_KEY_SIZE];
    alignas(16) uint8_t aligned_ctr[AES_BLOCK_SIZE];

    memcpy(aligned_key, g_aes_key, AES_KEY_SIZE);
    memcpy(aligned_ctr, g_aes_counter, AES_BLOCK_SIZE);

    // Process decryption in chunks
    size_t remaining = encrypted_size;
    size_t offset = 0;

    while (remaining > 0) {
        size_t chunk = min_size_t(remaining, CHUNK_SIZE);

        sgx_status_t ret = sgx_aes_ctr_decrypt(
            (const sgx_aes_ctr_128bit_key_t*)aligned_key,
            encrypted_data + offset,
            (uint32_t)chunk,
            aligned_ctr,
            128,  // Initial counter value
            decrypted_data + offset
        );

        if (ret != SGX_SUCCESS) {
            print_debug("Decryption failed with error code: %d\n", ret);
            return ret;
        }

        remaining -= chunk;
        offset += chunk;
    }

    // End timing decryption
    uint64_t current_time;
    if (ocall_get_current_time(&retval, &current_time) != SGX_SUCCESS) {
        return SGX_ERROR_UNEXPECTED;
    }
    g_timing.decryption_time = current_time - decrypt_start;

    // Verify decrypted data
    const TestData* test_data = (const TestData*)decrypted_data;
    if (test_data->version != CURRENT_VERSION) {
        print_debug("Error: Invalid version in decrypted data\n");
        return SGX_ERROR_UNEXPECTED;
    }

    if (test_data->count == 0 || test_data->count > MAX_VALUES) {
        print_debug("Error: Invalid count in decrypted data\n");
        return SGX_ERROR_UNEXPECTED;
    }

    return SGX_SUCCESS;
}

sgx_status_t ecall_check_number_encrypted(int number, uint8_t* encrypted_result, 
                                         size_t result_size) {
    if (!g_is_initialized || !g_secret_data || !encrypted_result || 
        result_size < AES_BLOCK_SIZE) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // Start total timing
    uint64_t retval;
    uint64_t start_time;
    if (ocall_get_current_time(&retval, &start_time) != SGX_SUCCESS) {
        return SGX_ERROR_UNEXPECTED;
    }
    g_timing.start_time = start_time;

    // Get the intersection check result
    int result = ecall_check_number(number);

    // End processing timing
    uint64_t process_end_time;
    if (ocall_get_current_time(&retval, &process_end_time) != SGX_SUCCESS) {
        return SGX_ERROR_UNEXPECTED;
    }
    g_timing.processing_time += process_end_time - start_time;

    // Convert result to byte for encryption
    uint8_t bool_result = (result == 1) ? 1 : 0;

    // Start encryption timing
    uint64_t encryption_start;
    if (ocall_get_current_time(&retval, &encryption_start) != SGX_SUCCESS) {
        return SGX_ERROR_UNEXPECTED;
    }

    // Ensure proper alignment
    alignas(16) uint8_t aligned_key[AES_KEY_SIZE];
    alignas(16) uint8_t aligned_ctr[AES_BLOCK_SIZE];

    memcpy(aligned_key, g_aes_key, AES_KEY_SIZE);
    memcpy(aligned_ctr, g_aes_counter, AES_BLOCK_SIZE);

    // Use SGX's native AES-CTR encryption
    sgx_status_t ret = sgx_aes_ctr_encrypt(
        (const sgx_aes_ctr_128bit_key_t*)aligned_key,
        &bool_result,
        1,  // Only encrypting 1 byte
        aligned_ctr,
        128,  // Initial counter value
        encrypted_result
    );

    // End encryption and total timing
    uint64_t current_time;
    if (ocall_get_current_time(&retval, &current_time) != SGX_SUCCESS) {
        return SGX_ERROR_UNEXPECTED;
    }

    g_timing.encryption_time += current_time - encryption_start;
    g_timing.total_time = current_time - g_timing.start_time;

    return ret;
}

int ecall_check_number(int number) {
    if (!g_is_initialized || !g_secret_data) {
        return -1;
    }

    if (g_secret_data->count == 0) {
        return -2;  // Error: empty data
    }

    // Linear search with page fault tracking
    for (int i = 0; i < g_secret_data->count; i++) {
        // Track page faults and ensure memory access is within enclave
        if (sgx_is_within_enclave(&g_secret_data->values[i], sizeof(int))) {
            g_page_fault_count++;
        }

        // Cache-friendly comparison
        int current_value = g_secret_data->values[i];
        if (current_value == number) {
            return 1;  // Match found
        }
    }

    return 0;  // No match found
}

sgx_status_t ecall_update_counter(const uint8_t* counter, size_t counter_size) {
    if (!counter || counter_size != AES_BLOCK_SIZE) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    memcpy(g_aes_counter, counter, AES_BLOCK_SIZE);
    return SGX_SUCCESS;
}

sgx_status_t ecall_get_page_fault_count(uint64_t* count) {
    if (!count) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    *count = g_page_fault_count;
    return SGX_SUCCESS;
}

sgx_status_t ecall_get_timing_info(uint64_t* encryption_time, 
                                  uint64_t* processing_time,
                                  uint64_t* total_time,
                                  uint64_t* decryption_time) {
    if (!encryption_time || !processing_time || !total_time || !decryption_time) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    *encryption_time = g_timing.encryption_time;
    *processing_time = g_timing.processing_time;
    *decryption_time = g_timing.decryption_time;
    *total_time = g_timing.total_time;

    return SGX_SUCCESS;
}

sgx_status_t ecall_reset_timing() {
    memset(&g_timing, 0, sizeof(TimingInfo));
    return SGX_SUCCESS;
}

void ecall_cleanup() {
    if (g_secret_data) {
        // Securely wipe secret data before freeing
        memset(g_secret_data, 0, sizeof(SecretData));
        free(g_secret_data);
        g_secret_data = nullptr;
    }

    // Clear all sensitive data
    memset(g_aes_key, 0, AES_KEY_SIZE);
    memset(g_aes_counter, 0, AES_BLOCK_SIZE);

    g_is_initialized = false;
    g_aes_initialized = false;
    g_page_fault_count = 0;
}
