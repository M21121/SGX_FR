// tools/vector_generator/vector_generator.cpp
#include "vector_generator.h"
#include <random>
#include <fstream>
#include <iostream>
#include <iomanip>

VectorGenerator::VectorGenerator(const GeneratorConfig& config) : config_(config) {
    if (config_.num_vectors > MAX_VECTORS) {
        std::cerr << "Warning: Number of vectors exceeds maximum (" << MAX_VECTORS 
                  << "). Limiting to maximum.\n";
        config_.num_vectors = MAX_VECTORS;
    }
}

vector_t VectorGenerator::generate_random_vector() {
    std::mt19937 gen(config_.seed);
    std::uniform_real_distribution<float> dist(config_.min_value, config_.max_value);

    vector_t vec;
    for (size_t i = 0; i < VECTOR_DIM; ++i) {
        vec[i] = dist(gen);
    }
    return vec;
}

vector_t VectorGenerator::generate_sequential_vector() {
    vector_t vec;
    float step = (config_.max_value - config_.min_value) / VECTOR_DIM;
    for (size_t i = 0; i < VECTOR_DIM; ++i) {
        vec[i] = config_.min_value + (step * i);
    }
    return vec;
}

vector_t VectorGenerator::generate_gaussian_vector() {
    std::mt19937 gen(config_.seed);
    std::normal_distribution<float> dist(config_.mean, config_.stddev);

    vector_t vec;
    for (size_t i = 0; i < VECTOR_DIM; ++i) {
        vec[i] = dist(gen);
    }
    return vec;
}

vector_t VectorGenerator::generate_uniform_vector() {
    vector_t vec;
    float value = config_.min_value;
    for (size_t i = 0; i < VECTOR_DIM; ++i) {
        vec[i] = value;
    }
    return vec;
}

vector_t VectorGenerator::generate_single_vector() {
    switch (config_.type) {
        case GenerationType::RANDOM:
            return generate_random_vector();
        case GenerationType::SEQUENTIAL:
            return generate_sequential_vector();
        case GenerationType::GAUSSIAN:
            return generate_gaussian_vector();
        case GenerationType::UNIFORM:
            return generate_uniform_vector();
        default:
            return vector_t();
    }
}

std::vector<vector_t> VectorGenerator::generate_vectors() {
    std::vector<vector_t> vectors;
    vectors.reserve(config_.num_vectors);

    for (size_t i = 0; i < config_.num_vectors; ++i) {
        config_.seed += i; // Ensure different vectors
        vectors.push_back(generate_single_vector());
    }

    return vectors;
}

bool VectorGenerator::save_to_file(const std::string& filename, const std::vector<vector_t>& vectors) {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    file << std::fixed << std::setprecision(6);
    for (const auto& vec : vectors) {
        for (size_t i = 0; i < VECTOR_DIM; ++i) {
            file << vec[i];
            if (i < VECTOR_DIM - 1) file << " ";
        }
        file << "\n";
    }

    return true;
}

bool VectorGenerator::save_single_vector(const std::string& filename, const vector_t& vector) {
    return save_to_file(filename, {vector});
}
