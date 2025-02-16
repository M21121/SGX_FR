// Enclave.h
#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <sgx_tcrypto.h>

#if defined(__cplusplus)
extern "C" {
#endif

int ecall_check_number(int number);
sgx_status_t ecall_initialize_secret_data(const uint8_t* sealed_data, size_t sealed_size);
sgx_status_t ecall_get_page_fault_count(uint64_t* count);
sgx_status_t ecall_decrypt_test_data(const uint8_t* encrypted_data, size_t encrypted_size,
                                    uint8_t* decrypted_data, size_t decrypted_size);
sgx_status_t ecall_initialize_aes_key(const uint8_t* key_data, size_t key_size);
sgx_status_t ecall_check_number_encrypted(int number, uint8_t* encrypted_result, size_t result_size);
sgx_status_t ecall_get_timing_info(uint64_t* encryption_time, 
    uint64_t* processing_time,
    uint64_t* total_time,
    uint64_t* decryption_time);

#if defined(__cplusplus)
}
#endif

#endif
