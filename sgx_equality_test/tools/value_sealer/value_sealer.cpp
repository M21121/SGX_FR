
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <sstream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include "../../common/shared_types.h"  // This now provides SecretData and TestData structs

#define AES_KEY_SIZE 16  // 128 bits for AES-CTR
#define AES_BLOCK_SIZE 16

void print_usage() {
    std::cout << "Usage: value_sealer [seal|seal-tests] output_file input_file\n";
    std::cout << "  seal       - Seal secret values to a file\n";
    std::cout << "  seal-tests - Seal and encrypt test values to a file\n";
    std::cout << "  output_file - Path to the output sealed data file\n";
    std::cout << "  input_file  - Path to the input text file containing numbers\n";
    std::cout << "Maximum supported values: " << MAX_VALUES << "\n";
    std::cout << "\nExample:\n";
    std::cout << "  ./value_sealer seal secret.dat numbers.txt\n";
    std::cout << "  ./value_sealer seal-tests test.dat test_numbers.txt\n";
}
bool read_or_generate_key(unsigned char* key, unsigned char* counter) {
    std::ifstream keyfile("aes.key", std::ios::binary);
    if (keyfile) {
        // Read existing key and counter
        keyfile.read(reinterpret_cast<char*>(key), AES_KEY_SIZE);
        keyfile.read(reinterpret_cast<char*>(counter), AES_BLOCK_SIZE);
        keyfile.close();
        std::cout << "Using existing AES key from 'aes.key'\n";
        return true;
    }

    // Generate random key
    if (RAND_bytes(key, AES_KEY_SIZE) != 1) {
        std::cerr << "Error: Failed to generate AES key\n";
        return false;
    }

    // Generate random counter
    if (RAND_bytes(counter, AES_BLOCK_SIZE) != 1) {
        std::cerr << "Error: Failed to generate counter\n";
        return false;
    }

    // Save key and counter for later use by the enclave
    std::ofstream newkeyfile("aes.key", std::ios::binary);
    if (!newkeyfile) {
        std::cerr << "Error: Failed to create key file\n";
        return false;
    }
    newkeyfile.write(reinterpret_cast<char*>(key), AES_KEY_SIZE);
    newkeyfile.write(reinterpret_cast<char*>(counter), AES_BLOCK_SIZE);
    newkeyfile.close();

    std::cout << "Generated and saved new AES key and counter to 'aes.key'\n";
    return true;
}

bool encrypt_data(const uint8_t* input, size_t input_size,
                 std::vector<uint8_t>& output,
                 const unsigned char* key,
                 const unsigned char* counter) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "Error: Failed to create cipher context\n";
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, counter) != 1) {
        std::cerr << "Error: Failed to initialize encryption\n";
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    output.resize(input_size + EVP_MAX_BLOCK_LENGTH);
    int encrypted_length = 0;
    int final_length = 0;

    if (EVP_EncryptUpdate(ctx, output.data(), &encrypted_length,
                         input, input_size) != 1) {
        std::cerr << "Error: Failed to encrypt data\n";
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_EncryptFinal_ex(ctx, output.data() + encrypted_length, &final_length) != 1) {
        std::cerr << "Error: Failed to finalize encryption\n";
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    EVP_CIPHER_CTX_free(ctx);
    output.resize(encrypted_length + final_length);
    return true;
}

bool read_numbers_from_file(const std::string& filename, std::vector<int>& numbers) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Number of values exceeds maximum limit of " << MAX_VALUES << "\n";
        return false;
    }

    std::string line;
    int line_number = 0;
    numbers.clear();

    while (std::getline(file, line)) {
        line_number++;

        if (line.empty() || line[0] == '#' || line[0] == '/') {
            continue;
        }

        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);
        int number;

        while (iss >> number) {
            numbers.push_back(number);

            if (numbers.size() > MAX_VALUES) {
                std::cerr << "Error: Number of values exceeds maximum limit of " << MAX_VALUES << "\n";
                return false;
            }
        }

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
        std::unique_ptr<SecretData> secret_data(new SecretData());
        secret_data->version = 1;

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

        for (size_t i = 0; i < sorted_values.size(); i++) {
            secret_data->values[i] = sorted_values[i];
        }

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
        std::unique_ptr<TestData> test_data(new TestData());
        test_data->version = 1;
        test_data->count = static_cast<uint32_t>(values.size());

        for (size_t i = 0; i < values.size(); i++) {
            test_data->values[i] = values[i];
        }

        // Use existing key or generate new one if it doesn't exist
        unsigned char key[AES_KEY_SIZE];
        unsigned char counter[AES_BLOCK_SIZE];
        if (!read_or_generate_key(key, counter)) {
            return false;
        }

        // Encrypt the test data
        std::vector<uint8_t> encrypted_data;
        if (!encrypt_data(reinterpret_cast<const uint8_t*>(test_data.get()),
                         sizeof(TestData),
                         encrypted_data,
                         key,
                         counter)) {
            return false;
        }

        // Write to file: counter + encrypted data
        std::ofstream file(output_file, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Failed to create output file: " << output_file << "\n";
            return false;
        }

        // Write counter first
        file.write(reinterpret_cast<const char*>(counter), AES_BLOCK_SIZE);

        // Write encrypted data
        file.write(reinterpret_cast<const char*>(encrypted_data.data()), encrypted_data.size());

        if (!file) {
            std::cerr << "Error: Failed to write to output file\n";
            return false;
        }

        file.close();
        std::cout << "Successfully sealed and encrypted " << values.size() 
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
