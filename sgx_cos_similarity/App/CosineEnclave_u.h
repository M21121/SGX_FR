#ifndef COSINEENCLAVE_U_H__
#define COSINEENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */


#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif


sgx_status_t ecall_initialize_reference_vectors(sgx_enclave_id_t eid, sgx_status_t* retval, const uint8_t* sealed_data, size_t sealed_size);
sgx_status_t ecall_compute_cosine_similarity(sgx_enclave_id_t eid, float* retval, const float* query_vector);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
