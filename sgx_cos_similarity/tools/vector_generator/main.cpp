// tools/vector_generator/main.cpp
#include "vector_generator.h"
#include <iostream>
#include <string>
#include <ctime>

void print_usage() {
    std::cout << "Usage: vector_generator <output_file> [options]\n"
              << "Options:\n"
              << "  --type <random|sequential|gaussian|uniform>  Generation type (default: random)\n"
              << "  --count <n>                                 Number of vectors (default: 1)\n"
              << "  --min <value>                              Minimum value (default: -1.0)\n"
              << "  --max <value>                              Maximum value (default: 1.0)\n"
              << "  --mean <value>                             Mean for Gaussian (default: 0.0)\n"
              << "  --stddev <value>                           StdDev for Gaussian (default: 1.0)\n"
              << "  --seed <value>                             Random seed (default: time-based)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string output_file = argv[1];

    // Default configuration
    GeneratorConfig config{
        .type = GenerationType::RANDOM,
        .num_vectors = 1,
        .min_value = -1.0f,
        .max_value = 1.0f,
        .mean = 0.0f,
        .stddev = 1.0f,
        .seed = static_cast<unsigned int>(std::time(nullptr))
    };

    // Parse command line arguments
    for (int i = 2; i < argc; i += 2) {
        std::string arg = argv[i];
        if (i + 1 >= argc) {
            std::cerr << "Missing value for argument: " << arg << std::endl;
            return 1;
        }

        if (arg == "--type") {
            std::string type = argv[i + 1];
            if (type == "random") config.type = GenerationType::RANDOM;
            else if (type == "sequential") config.type = GenerationType::SEQUENTIAL;
            else if (type == "gaussian") config.type = GenerationType::GAUSSIAN;
            else if (type == "uniform") config.type = GenerationType::UNIFORM;
            else {
                std::cerr << "Invalid type: " << type << std::endl;
                return 1;
            }
        }
        else if (arg == "--count") config.num_vectors = std::stoul(argv[i + 1]);
        else if (arg == "--min") config.min_value = std::stof(argv[i + 1]);
        else if (arg == "--max") config.max_value = std::stof(argv[i + 1]);
        else if (arg == "--mean") config.mean = std::stof(argv[i + 1]);
        else if (arg == "--stddev") config.stddev = std::stof(argv[i + 1]);
        else if (arg == "--seed") config.seed = std::stoul(argv[i + 1]);
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return 1;
        }
    }

    VectorGenerator generator(config);
    std::vector<vector_t> vectors = generator.generate_vectors();

    if (!generator.save_to_file(output_file, vectors)) {
        std::cerr << "Failed to save vectors to file" << std::endl;
        return 1;
    }

    std::cout << "Generated " << vectors.size() << " vectors to " << output_file << std::endl;
    return 0;
}
