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

sgx_status_t ecall_initialize_secret_data(const uint8_t* sealed_data, size_t sealed_size)
{
    if (!sealed_data || sealed_size != sizeof(SecretData)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // Dynamically allocate memory for the large array
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

    // Binary search for better performance with large arrays
    int left = 0;
    int right = g_secret_data->count - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
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

// Add destructor to clean up memory
void ecall_cleanup()
{
    if (g_secret_data) {
        free(g_secret_data);
        g_secret_data = nullptr;
    }
    g_is_initialized = false;
}
