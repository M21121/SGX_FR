#include "CosineEnclave_u.h"
#include <errno.h>

typedef struct ms_ecall_initialize_reference_vectors_t {
	sgx_status_t ms_retval;
	const uint8_t* ms_sealed_data;
	size_t ms_sealed_size;
} ms_ecall_initialize_reference_vectors_t;

typedef struct ms_ecall_compute_cosine_similarity_t {
	float ms_retval;
	const float* ms_query_vector;
} ms_ecall_compute_cosine_similarity_t;

static const struct {
	size_t nr_ocall;
	void * table[1];
} ocall_table_CosineEnclave = {
	0,
	{ NULL },
};
sgx_status_t ecall_initialize_reference_vectors(sgx_enclave_id_t eid, sgx_status_t* retval, const uint8_t* sealed_data, size_t sealed_size)
{
	sgx_status_t status;
	ms_ecall_initialize_reference_vectors_t ms;
	ms.ms_sealed_data = sealed_data;
	ms.ms_sealed_size = sealed_size;
	status = sgx_ecall(eid, 0, &ocall_table_CosineEnclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t ecall_compute_cosine_similarity(sgx_enclave_id_t eid, float* retval, const float* query_vector)
{
	sgx_status_t status;
	ms_ecall_compute_cosine_similarity_t ms;
	ms.ms_query_vector = query_vector;
	status = sgx_ecall(eid, 1, &ocall_table_CosineEnclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

