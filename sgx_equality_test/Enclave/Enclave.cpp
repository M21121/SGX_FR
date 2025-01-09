// Enclave.cpp
#include "Enclave_t.h"
#include <sgx_trts.h>
#include <string.h>

struct SecretData {
    uint32_t version;
    uint32_t count;
    int values[1024];
};

static SecretData g_secret_data;
static bool g_is_initialized = false;

sgx_status_t ecall_initialize_secret_data(const uint8_t* sealed_data, size_t sealed_size)
{
    if (!sealed_data || sealed_size != sizeof(SecretData)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    memcpy(&g_secret_data, sealed_data, sizeof(SecretData));
    g_is_initialized = true;
    return SGX_SUCCESS;
}

int ecall_check_number(int number)
{
    if (!g_is_initialized) {
        return -1;
    }

    for (uint32_t i = 0; i < g_secret_data.count; i++) {
        if (g_secret_data.values[i] == number) {
            return 1;
        }
    }
    return 0;
}
