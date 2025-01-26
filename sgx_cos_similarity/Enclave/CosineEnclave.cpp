// CosineEnclave.cpp
#include "CosineEnclave_t.h"
#include <sgx_trts.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdlib.h>
#include "../common/shared_types.h"

// Static enclave data
static ReferenceVectors* g_reference_vectors = nullptr;
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
    if (!sealed_data || sealed_size < sizeof(uint32_t) * 2) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // First read version and count
    uint32_t version, count;
    memcpy(&version, sealed_data, sizeof(uint32_t));
    memcpy(&count, sealed_data + sizeof(uint32_t), sizeof(uint32_t));

    if (version != CURRENT_VERSION || count == 0 || count > MAX_VECTORS) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // Calculate total required size
    size_t required_size = sizeof(uint32_t) * 2 + (count * sizeof(vector_t));
    if (sealed_size != required_size) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    // Clean up any existing data
    if (g_reference_vectors) {
        if (g_reference_vectors->vectors) {
            delete[] g_reference_vectors->vectors;
        }
        delete g_reference_vectors;
        g_reference_vectors = nullptr;
    }

    // Allocate memory for the reference vectors
    try {
        g_reference_vectors = new ReferenceVectors();
        g_reference_vectors->version = version;
        g_reference_vectors->count = count;
        g_reference_vectors->vectors = new vector_t[count];

        // Copy the vectors data
        memcpy(g_reference_vectors->vectors, 
               sealed_data + (sizeof(uint32_t) * 2), 
               count * sizeof(vector_t));

        g_is_initialized = true;
        return SGX_SUCCESS;
    } catch (...) {
        if (g_reference_vectors) {
            if (g_reference_vectors->vectors) {
                delete[] g_reference_vectors->vectors;
            }
            delete g_reference_vectors;
            g_reference_vectors = nullptr;
        }
        g_is_initialized = false;
        return SGX_ERROR_OUT_OF_MEMORY;
    }
}

float ecall_compute_cosine_similarity(const float* query_vector)
{
    if (!g_is_initialized || !query_vector || !g_reference_vectors) {
        return -2.0f; // Error value
    }

    float query_magnitude = compute_magnitude(query_vector);
    if (is_effectively_zero(query_magnitude)) {
        return -1.0f; // Zero vector
    }

    float max_similarity = -1.0f;

    for (uint32_t i = 0; i < g_reference_vectors->count; i++) {
        float ref_magnitude = compute_vector_magnitude(g_reference_vectors->vectors[i]);
        if (is_effectively_zero(ref_magnitude)) continue; // Skip zero vectors

        float dot_product = compute_dot_product(query_vector, g_reference_vectors->vectors[i]);
        float similarity = dot_product / (query_magnitude * ref_magnitude);

        if (similarity > max_similarity) {
            max_similarity = similarity;
        }
    }

    return max_similarity;
}

void ecall_cleanup_reference_vectors()
{
    if (g_reference_vectors) {
        if (g_reference_vectors->vectors) {
            delete[] g_reference_vectors->vectors;
        }
        delete g_reference_vectors;
        g_reference_vectors = nullptr;
    }
    g_is_initialized = false;
}
