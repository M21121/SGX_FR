// Enclave.h
#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#if defined(__cplusplus)
extern "C" {
#endif

int ecall_check_number(int number);
sgx_status_t ecall_initialize_secret_data(const uint8_t* sealed_data, size_t sealed_size);

#if defined(__cplusplus)
}
#endif

#endif
