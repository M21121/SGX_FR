#ifndef COSINEENCLAVE_T_H__
#define COSINEENCLAVE_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */


#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

sgx_status_t ecall_initialize_reference_vectors(const uint8_t* sealed_data, size_t sealed_size);
float ecall_compute_cosine_similarity(const float* query_vector);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
