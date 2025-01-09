// tools/vector_generator/vector_generator.h
#ifndef VECTOR_GENERATOR_H
#define VECTOR_GENERATOR_H

#include <vector>
#include <string>
#include "../../common/shared_types.h"

enum class GenerationType {
    RANDOM,
    SEQUENTIAL,
    GAUSSIAN,
    UNIFORM
};

struct GeneratorConfig {
    GenerationType type;
    size_t num_vectors;
    float min_value;
    float max_value;
    float mean;      // For Gaussian distribution
    float stddev;    // For Gaussian distribution
    unsigned int seed;
};

class VectorGenerator {
public:
    explicit VectorGenerator(const GeneratorConfig& config);

    std::vector<vector_t> generate_vectors();
    vector_t generate_single_vector();

    bool save_to_file(const std::string& filename, const std::vector<vector_t>& vectors);
    bool save_single_vector(const std::string& filename, const vector_t& vector);

private:
    GeneratorConfig config_;
    vector_t generate_random_vector();
    vector_t generate_sequential_vector();
    vector_t generate_gaussian_vector();
    vector_t generate_uniform_vector();
};

#endif // VECTOR_GENERATOR_H
