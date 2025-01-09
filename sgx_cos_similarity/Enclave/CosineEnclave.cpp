// CosineEnclave.cpp
#include "CosineEnclave_t.h"
#include <sgx_trts.h>
#include <array>
#include <cmath>
#include <cstdint>
#include "../common/shared_types.h"  // Add this include for CURRENT_VERSION and MAX_VECTORS

// Remove these as they're now defined in shared_types.h
// #define VECTOR_DIM 512
// typedef std::array<float, VECTOR_DIM> vector_t

// Use the ReferenceVectors struct from shared_types.h
static ReferenceVectors g_reference_vectors;
static bool g_is_initialized = false;

// Helper functions
static float compute_dot_product(const float* vec1, const vector_t& vec2) {
    float dot_product = 0.0f;
    for (size_t i = 0; i < VECTOR_DIM; i++) {
        dot_product += vec1[i] * vec2[i];
    }
    return dot_product;
}

static float compute_magnitude(const float* vec) {
    float sum_squares = 0.0f;
    for (size_t i = 0; i < VECTOR_DIM; i++) {
        sum_squares += vec[i] * vec[i];
    }
    return std::sqrt(sum_squares);
}

static float compute_vector_magnitude(const vector_t& vec) {
    float sum_squares = 0.0f;
    for (size_t i = 0; i < VECTOR_DIM; i++) {
        sum_squares += vec[i] * vec[i];
    }
    return std::sqrt(sum_squares);
}

static bool is_effectively_zero(float value) {
    const float epsilon = 1e-6f;
    return std::abs(value) < epsilon;
}

sgx_status_t ecall_initialize_reference_vectors(const uint8_t* sealed_data, size_t sealed_size)
{
    if (!sealed_data || sealed_size != sizeof(ReferenceVectors)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // Check if we have enough memory
    if (sealed_size > UINT32_MAX) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // Verify the data before copying
    const ReferenceVectors* temp_ref = reinterpret_cast<const ReferenceVectors*>(sealed_data);
    if (temp_ref->version != CURRENT_VERSION || 
        temp_ref->count == 0 || 
        temp_ref->count > MAX_VECTORS) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // Use try-catch to handle potential memory issues
    try {
        memcpy(&g_reference_vectors, sealed_data, sealed_size);
        g_is_initialized = true;
        return SGX_SUCCESS;
    } catch (...) {
        return SGX_ERROR_OUT_OF_MEMORY;
    }
}

float ecall_compute_cosine_similarity(const float* query_vector)
{
    if (!g_is_initialized || !query_vector) {
        return -2.0f; // Error value
    }

    float query_magnitude = compute_magnitude(query_vector);
    if (is_effectively_zero(query_magnitude)) {
        return -1.0f; // Zero vector
    }

    float max_similarity = -1.0f;

    for (uint32_t i = 0; i < g_reference_vectors.count; i++) {
        float ref_magnitude = compute_vector_magnitude(g_reference_vectors.vectors[i]);
        if (is_effectively_zero(ref_magnitude)) continue; // Skip zero vectors

        float dot_product = compute_dot_product(query_vector, g_reference_vectors.vectors[i]);
        float similarity = dot_product / (query_magnitude * ref_magnitude);

        if (similarity > max_similarity) {
            max_similarity = similarity;
        }
    }

    return max_similarity;
}
