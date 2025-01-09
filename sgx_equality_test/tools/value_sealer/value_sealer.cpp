#include <iostream>
#include <fstream>
#include <vector>
#include <string>


struct SecretData {
    uint32_t version;
    uint32_t count;
    int values[1024];
};

struct TestData {
    uint32_t version;
    uint32_t count;
    int values[1024];
};

void print_usage() {
    std::cout << "Usage: value_sealer [seal|seal-tests] output_file [values...]\n";
    std::cout << "  seal       - Seal secret values to a file\n";
    std::cout << "  seal-tests - Seal test values to a file\n";
}

bool seal_values(const std::string& output_file, const std::vector<int>& values) {
    if (values.empty() || values.size() > 1024) {
        std::cerr << "Invalid number of values (must be between 1 and 1024)\n";
        return false;
    }

    SecretData secret_data;
    secret_data.version = 1;
    secret_data.count = values.size();

    for (size_t i = 0; i < values.size(); i++) {
        secret_data.values[i] = values[i];
    }

    std::ofstream file(output_file, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open output file\n";
        return false;
    }

    file.write(reinterpret_cast<const char*>(&secret_data), sizeof(SecretData));
    file.close();

    return true;
}

bool seal_test_values(const std::string& output_file, const std::vector<int>& values) {
    if (values.empty() || values.size() > 1024) {
        std::cerr << "Invalid number of test values (must be between 1 and 1024)\n";
        return false;
    }

    TestData test_data;
    test_data.version = 1;
    test_data.count = values.size();

    for (size_t i = 0; i < values.size(); i++) {
        test_data.values[i] = values[i];
    }

    std::ofstream file(output_file, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open output file\n";
        return false;
    }

    file.write(reinterpret_cast<const char*>(&test_data), sizeof(TestData));
    file.close();

    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];
    std::string output_file = argv[2];

    std::vector<int> values;
    for (int i = 3; i < argc; i++) {
        values.push_back(std::stoi(argv[i]));
    }

    if (command == "seal") {
        if (seal_values(output_file, values)) {
            std::cout << "Secret values sealed successfully to " << output_file << "\n";
            return 0;
        }
        return 1;
    } 
    else if (command == "seal-tests") {
        if (seal_test_values(output_file, values)) {
            std::cout << "Test values sealed successfully to " << output_file << "\n";
            return 0;
        }
        return 1;
    }
    else {
        print_usage();
        return 1;
    }
}
