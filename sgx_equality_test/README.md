# SGX Equality Testing Program

This program demonstrates secure number matching using Intel SGX enclaves. It checks if provided test numbers match any numbers in a sealed secret list.

## Prerequisites
- Intel SGX SDK
- Intel SGX PSW
- g++ compiler
- Linux OS with SGX support

## Setup Steps
1. Generate Enclave Private Key
   If you don't have a private key for signing the enclave:

```Bash 
openssl genrsa -out Enclave/Enclave_private.pem -3 3072
```

2. Build the Program
   a. Build the main program:
      
```Bash 
make clean
make
```

   b. Build the value sealer tool:

```Bash
cd tools/value_sealer
make clean
make
cd ../..
```

3. Create Sealed Data Files
   a. Create a file with secret numbers:
      
```Bash
./tools/value_sealer/value_sealer seal tools/sealed_data/secret_numbers.dat 42 123 456 789 # (Replace numbers with your desired secret values)
```

   b. Create a file with test numbers:
      
```Bash
./tools/value_sealer/value_sealer seal-tests tools/sealed_data/test_numbers.dat 42 100 200 456 # (Replace numbers with values you want to test)
```

4. Run the Program
   
```Bash
./sgx_equality_test
```

## Cleaning Up
- Clean main program:
  
```Bash
make clean
```

- Clean value sealer:
  
```Bash
cd tools/value_sealer
make clean
cd ../..
```

- Remove sealed data:
  
```Bash
rm tools/sealed_data/*.dat
```
