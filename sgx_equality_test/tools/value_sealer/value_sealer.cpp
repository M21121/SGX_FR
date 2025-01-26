#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <sstream>

struct SecretData {
    uint32_t version;
    uint32_t count;
    int values[33554432];  // 2^25 numbers
};

struct TestData {
    uint32_t version;
    uint32_t count;
    int values[33554432];  // 2^25 numbers
};

void print_usage() {
    std::cout << "Usage: value_sealer [seal|seal-tests] output_file input_file\n";
    std::cout << "  seal       - Seal secret values to a file\n";
    std::cout << "  seal-tests - Seal test values to a file\n";
    std::cout << "  output_file - Path to the output sealed data file\n";
    std::cout << "  input_file  - Path to the input text file containing numbers\n";
    std::cout << "Maximum supported values: 33,554,432\n";
    std::cout << "\nExample:\n";
    std::cout << "  ./value_sealer seal secret.dat numbers.txt\n";
    std::cout << "  ./value_sealer seal-tests test.dat test_numbers.txt\n";
}

bool read_numbers_from_file(const std::string& filename, std::vector<int>& numbers) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Cannot open input file: " << filename << "\n";
        return false;
    }

    std::string line;
    int line_number = 0;
    numbers.clear();

    while (std::getline(file, line)) {
        line_number++;

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == '/') {
            continue;
        }

        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);
        int number;

        while (iss >> number) {
            numbers.push_back(number);

            if (numbers.size() > 33554432) {
                std::cerr << "Error: Number of values exceeds maximum limit of 33,554,432\n";
                return false;
            }
        }

        // Check if the line was parsed successfully
        if (iss.fail() && !iss.eof()) {
            std::cerr << "Error: Invalid number format at line " << line_number << ": " << line << "\n";
            return false;
        }
    }

    if (numbers.empty()) {
        std::cerr << "Error: No valid numbers found in file\n";
        return false;
    }

    std::cout << "Successfully read " << numbers.size() << " numbers from " << filename << "\n";
    return true;
}

bool seal_values(const std::string& output_file, const std::vector<int>& values) {
    try {
        auto secret_data = std::make_unique<SecretData>();
        secret_data->version = 1;

        // Sort values and remove duplicates
        std::vector<int> sorted_values = values;
        std::sort(sorted_values.begin(), sorted_values.end());
        sorted_values.erase(
            std::unique(sorted_values.begin(), sorted_values.end()),
            sorted_values.end()
        );

        if (sorted_values.size() != values.size()) {
            std::cout << "Info: Removed " << (values.size() - sorted_values.size()) 
                     << " duplicate values\n";
        }

        secret_data->count = static_cast<uint32_t>(sorted_values.size());

        // Copy sorted unique values
        for (size_t i = 0; i < sorted_values.size(); i++) {
            secret_data->values[i] = sorted_values[i];
        }

        // Write to file
        std::ofstream file(output_file, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Failed to create output file: " << output_file << "\n";
            return false;
        }

        file.write(reinterpret_cast<const char*>(secret_data.get()), sizeof(SecretData));

        if (!file) {
            std::cerr << "Error: Failed to write to output file\n";
            return false;
        }

        file.close();
        std::cout << "Successfully sealed " << sorted_values.size() 
                 << " unique values to " << output_file << "\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
}

bool seal_test_values(const std::string& output_file, const std::vector<int>& values) {
    try {
        auto test_data = std::make_unique<TestData>();
        test_data->version = 1;
        test_data->count = static_cast<uint32_t>(values.size());

        // Copy values directly (no sorting/deduplication for test values)
        for (size_t i = 0; i < values.size(); i++) {
            test_data->values[i] = values[i];
        }

        // Write to file
        std::ofstream file(output_file, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Failed to create output file: " << output_file << "\n";
            return false;
        }

        file.write(reinterpret_cast<const char*>(test_data.get()), sizeof(TestData));

        if (!file) {
            std::cerr << "Error: Failed to write to output file\n";
            return false;
        }

        file.close();
        std::cout << "Successfully sealed " << values.size() 
                 << " test values to " << output_file << "\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];
    std::string output_file = argv[2];
    std::string input_file = argv[3];

    if (command == "--help" || command == "-h") {
        print_usage();
        return 0;
    }

    std::vector<int> numbers;
    if (!read_numbers_from_file(input_file, numbers)) {
        return 1;
    }

    bool success = false;
    if (command == "seal") {
        success = seal_values(output_file, numbers);
    }
    else if (command == "seal-tests") {
        success = seal_test_values(output_file, numbers);
    }
    else {
        std::cerr << "Error: Unknown command: " << command << "\n";
        print_usage();
        return 1;
    }

    return success ? 0 : 1;
}
