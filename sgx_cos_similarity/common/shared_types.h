// ./common/shared_types.h
#ifndef _SHARED_TYPES_H_
#define _SHARED_TYPES_H_

#include <stdint.h>
#include <array>

#define VECTOR_DIM 512
#define MAX_VECTORS 65536  // Increased from 1024 to 65536
#define CURRENT_VERSION 1

typedef std::array<float, VECTOR_DIM> vector_t;

// Modified to use dynamic allocation for vectors
struct ReferenceVectors {
    uint32_t version;
    uint32_t count;
    vector_t* vectors;  // Changed from fixed array to pointer
};

struct QueryVector {
    uint32_t version;
    uint32_t count;
    vector_t vector;
};

#endif // _SHARED_TYPES_H_
