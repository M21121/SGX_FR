#ifndef _VECTOR_SEALER_H_
#define _VECTOR_SEALER_H_

#include <string>
#include <vector>
#include "../../common/shared_types.h"

bool seal_reference_vectors(const std::string& output_file, const std::vector<vector_t>& vectors);
bool seal_query_vector(const std::string& output_file, const vector_t& vector);
void print_usage();

#endif
