#ifndef _COSINE_ENCLAVE_H_
#define _COSINE_ENCLAVE_H_

#include <sgx_trts.h>

#if defined(__cplusplus)
extern "C" {
#endif

float ecall_compute_cosine_similarity(const float* query_vector);
sgx_status_t ecall_initialize_reference_vectors(const uint8_t* sealed_data, size_t sealed_size);

#if defined(__cplusplus)
}
#endif

#endif // _COSINE_ENCLAVE_H_
