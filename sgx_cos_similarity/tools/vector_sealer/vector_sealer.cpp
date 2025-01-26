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

    // Write header and vectors separately
    uint32_t version = CURRENT_VERSION;
    uint32_t count = vectors.size();

    std::ofstream file(output_file, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open output file\n";
        return false;
    }

    // Write version and count
    file.write(reinterpret_cast<const char*>(&version), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

    // Write vectors
    for (const auto& vec : vectors) {
        file.write(reinterpret_cast<const char*>(vec.data()), sizeof(vector_t));
    }

    std::cout << "Sealed " << count << " vectors (" 
              << (sizeof(uint32_t) * 2 + count * sizeof(vector_t)) 
              << " bytes) to " << output_file << "\n";

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
    std::cout << "Sealed query vector (" << sizeof(QueryVector) << " bytes) to " 
              << output_file << "\n";
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
            std::cout << "Reference vectors sealed successfully\n";
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
            std::cout << "Query vector sealed successfully\n";
            return 0;
        }
    }
    else {
        print_usage();
        return 1;
    }

    return 1;
}
