#include "vector_sealer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

void print_usage() {
    std::cout << "Usage: vector_sealer [seal-ref|seal-query] output_file input_file\n";
    std::cout << "  seal-ref   - Seal reference vectors to a file\n";
    std::cout << "  seal-query - Seal query vector to a file\n";
    std::cout << "\nInput file format:\n";
    std::cout << "  - One vector per line\n";
    std::cout << "  - Each line contains " << VECTOR_DIM << " space-separated float values\n";
}

bool read_vector(std::istream& input, vector_t& vector) {
    for (size_t i = 0; i < VECTOR_DIM; i++) {
        if (!(input >> vector[i])) {
            return false;
        }
    }
    return true;
}

bool read_vectors_from_file(const std::string& input_file, std::vector<vector_t>& vectors) {
    std::ifstream file(input_file);
    if (!file) {
        std::cerr << "Failed to open input file: " << input_file << "\n";
        return false;
    }

    vector_t current_vector;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments

        std::istringstream iss(line);
        if (read_vector(iss, current_vector)) {
            vectors.push_back(current_vector);
        } else {
            std::cerr << "Error: Invalid vector format\n";
            return false;
        }
    }

    return !vectors.empty();
}

bool seal_reference_vectors(const std::string& output_file, const std::vector<vector_t>& vectors) {
    if (vectors.empty() || vectors.size() > MAX_VECTORS) {
        std::cerr << "Invalid number of vectors (must be between 1 and " << MAX_VECTORS << ")\n";
        return false;
    }

    ReferenceVectors ref_data = {};  // Zero-initialize
    ref_data.version = CURRENT_VERSION;
    ref_data.count = vectors.size();

    for (size_t i = 0; i < vectors.size(); i++) {
        ref_data.vectors[i] = vectors[i];
    }

    std::ofstream file(output_file, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open output file\n";
        return false;
    }

    file.write(reinterpret_cast<const char*>(&ref_data), sizeof(ReferenceVectors));
    return true;
}

bool seal_query_vector(const std::string& output_file, const vector_t& vector) {
    QueryVector query_data = {};  // Zero-initialize
    query_data.version = CURRENT_VERSION;
    query_data.count = 1;
    query_data.vector = vector;

    std::ofstream file(output_file, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open output file\n";
        return false;
    }

    file.write(reinterpret_cast<const char*>(&query_data), sizeof(QueryVector));
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];
    std::string output_file = argv[2];
    std::string input_file = argv[3];

    if (command == "seal-ref") {
        std::vector<vector_t> vectors;
        if (!read_vectors_from_file(input_file, vectors)) {
            return 1;
        }

        if (seal_reference_vectors(output_file, vectors)) {
            std::cout << "Reference vectors sealed successfully to " << output_file << "\n";
            std::cout << "Number of vectors: " << vectors.size() << "\n";
            return 0;
        }
    } 
    else if (command == "seal-query") {
        std::vector<vector_t> vectors;
        if (!read_vectors_from_file(input_file, vectors)) {
            return 1;
        }

        if (vectors.size() != 1) {
            std::cerr << "Error: Query file must contain exactly one vector\n";
            return 1;
        }

        if (seal_query_vector(output_file, vectors[0])) {
            std::cout << "Query vector sealed successfully to " << output_file << "\n";
            return 0;
        }
    }
    else {
        print_usage();
        return 1;
    }

    return 1;
}
