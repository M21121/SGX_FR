// shared_types.h
#ifndef _SHARED_TYPES_H_
#define _SHARED_TYPES_H_

#include <stdint.h>

#define MAX_VALUES 33554432  // Changed from 1024 to 33554432
#define CURRENT_VERSION 1

struct SecretData {
    uint32_t version;
    uint32_t count;
    int values[MAX_VALUES];
};

struct TestData {
    uint32_t version;
    uint32_t count;
    int values[MAX_VALUES];
};

#endif // _SHARED_TYPES_H_
