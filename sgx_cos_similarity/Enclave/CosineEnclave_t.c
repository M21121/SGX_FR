#include "CosineEnclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */
#include "sgx_lfence.h" /* for sgx_lfence */

#include <errno.h>
#include <mbusafecrt.h> /* for memcpy_s etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_ENCLAVE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_within_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define ADD_ASSIGN_OVERFLOW(a, b) (	\
	((a) += (b)) < (b)	\
)


typedef struct ms_ecall_initialize_reference_vectors_t {
	sgx_status_t ms_retval;
	const uint8_t* ms_sealed_data;
	size_t ms_sealed_size;
} ms_ecall_initialize_reference_vectors_t;

typedef struct ms_ecall_compute_cosine_similarity_t {
	float ms_retval;
	const float* ms_query_vector;
} ms_ecall_compute_cosine_similarity_t;

static sgx_status_t SGX_CDECL sgx_ecall_initialize_reference_vectors(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_initialize_reference_vectors_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_initialize_reference_vectors_t* ms = SGX_CAST(ms_ecall_initialize_reference_vectors_t*, pms);
	ms_ecall_initialize_reference_vectors_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_initialize_reference_vectors_t), ms, sizeof(ms_ecall_initialize_reference_vectors_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const uint8_t* _tmp_sealed_data = __in_ms.ms_sealed_data;
	size_t _tmp_sealed_size = __in_ms.ms_sealed_size;
	size_t _len_sealed_data = _tmp_sealed_size;
	uint8_t* _in_sealed_data = NULL;
	sgx_status_t _in_retval;

	CHECK_UNIQUE_POINTER(_tmp_sealed_data, _len_sealed_data);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_sealed_data != NULL && _len_sealed_data != 0) {
		if ( _len_sealed_data % sizeof(*_tmp_sealed_data) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_sealed_data = (uint8_t*)malloc(_len_sealed_data);
		if (_in_sealed_data == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_sealed_data, _len_sealed_data, _tmp_sealed_data, _len_sealed_data)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	_in_retval = ecall_initialize_reference_vectors((const uint8_t*)_in_sealed_data, _tmp_sealed_size);
	if (memcpy_verw_s(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	if (_in_sealed_data) free(_in_sealed_data);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_compute_cosine_similarity(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_compute_cosine_similarity_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_compute_cosine_similarity_t* ms = SGX_CAST(ms_ecall_compute_cosine_similarity_t*, pms);
	ms_ecall_compute_cosine_similarity_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_compute_cosine_similarity_t), ms, sizeof(ms_ecall_compute_cosine_similarity_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const float* _tmp_query_vector = __in_ms.ms_query_vector;
	size_t _len_query_vector = 512 * sizeof(float);
	float* _in_query_vector = NULL;
	float _in_retval;

	if (sizeof(*_tmp_query_vector) != 0 &&
		512 > (SIZE_MAX / sizeof(*_tmp_query_vector))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_query_vector, _len_query_vector);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_query_vector != NULL && _len_query_vector != 0) {
		if ( _len_query_vector % sizeof(*_tmp_query_vector) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_query_vector = (float*)malloc(_len_query_vector);
		if (_in_query_vector == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_query_vector, _len_query_vector, _tmp_query_vector, _len_query_vector)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	_in_retval = ecall_compute_cosine_similarity((const float*)_in_query_vector);
	if (memcpy_verw_s(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	if (_in_query_vector) free(_in_query_vector);
	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv; uint8_t is_switchless;} ecall_table[2];
} g_ecall_table = {
	2,
	{
		{(void*)(uintptr_t)sgx_ecall_initialize_reference_vectors, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_compute_cosine_similarity, 0, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
} g_dyn_entry_table = {
	0,
};


