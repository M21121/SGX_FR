// Enclave.cpp
#include "Enclave_t.h"
#include <sgx_trts.h>
#include <string.h>

struct SecretData {
    uint32_t version;
    uint32_t count;
    int values[33554432];  // Updated size
};

static SecretData* g_secret_data = nullptr;
static bool g_is_initialized = false;
static uint64_t g_page_fault_count = 0;

sgx_status_t ecall_initialize_secret_data(const uint8_t* sealed_data, size_t sealed_size)
{
    if (!sealed_data || sealed_size != sizeof(SecretData)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    if (g_secret_data == nullptr) {
        g_secret_data = (SecretData*)malloc(sizeof(SecretData));
        if (!g_secret_data) {
            return SGX_ERROR_OUT_OF_MEMORY;
        }
    }

    memcpy(g_secret_data, sealed_data, sizeof(SecretData));
    g_is_initialized = true;
    return SGX_SUCCESS;
}

int ecall_check_number(int number)
{
    if (!g_is_initialized || !g_secret_data) {
        return -1;
    }

    if (g_secret_data->count == 0) {
        return -2;  // Error: empty data
    }

    // Binary search
    int left = 0;
    int right = g_secret_data->count - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        // Track page faults
        if (sgx_is_within_enclave(&g_secret_data->values[mid], sizeof(int))) {
            g_page_fault_count++;
        }

        if (g_secret_data->values[mid] == number) {
            return 1;
        }
        if (g_secret_data->values[mid] < number) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return 0;
}

sgx_status_t ecall_get_page_fault_count(uint64_t* count)
{
    if (!count) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    *count = g_page_fault_count;
    return SGX_SUCCESS;
}

void ecall_cleanup()
{
    if (g_secret_data) {
        free(g_secret_data);
        g_secret_data = nullptr;
    }
    g_is_initialized = false;
    g_page_fault_count = 0;
}
