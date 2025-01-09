// shared_types.h
#ifndef _SHARED_TYPES_H_
#define _SHARED_TYPES_H_

#include <stdint.h>
#include <array>

#define VECTOR_DIM 512
#define MAX_VECTORS 1024
#define CURRENT_VERSION 1

typedef std::array<float, VECTOR_DIM> vector_t;

struct ReferenceVectors {
    uint32_t version;
    uint32_t count;
    vector_t vectors[MAX_VECTORS];
};

struct QueryVector {
    uint32_t version;
    uint32_t count;
    vector_t vector;
};

#endif // _SHARED_TYPES_H_
