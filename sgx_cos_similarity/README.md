# SGX Cosine Similarity Computation System


Build Requirements
----------------
- Intel SGX SDK (version 2.15 or later)
- C++11 compatible compiler
- Make build system
- Linux operating system

## Building the System
1. Set up SGX environment variables:

```Bash
 source ${SGX_SDK}/environment
```

2. Build the main application:

```Bash
make
```

3. Build the tools:

```Bash
cd tools/vector_generator && make
cd ../vector_sealer && make
```

## Usage Process
1. Generate Test Vectors:
   
```Bash
./tools/vector_generator/vector_generator output_file [options]
```   

Options:
   --type <random|sequential|gaussian|uniform>  Generation type (default: random)
   --count <n>                                 Number of vectors (default: 1)
   --min <value>                              Minimum value (default: -1.0)
   --max <value>                              Maximum value (default: 1.0)
   --mean <value>                             Mean for Gaussian (default: 0.0)
   --stddev <value>                           StdDev for Gaussian (default: 1.0)
   --seed <value>                             Random seed (default: time-based)

Example:

```Bash
./tools/vector_generator/vector_generator reference_vectors.txt --type random --count 100
```

```Bash
./tools/vector_generator/vector_generator query_vector.txt --type random --count 1
```

2. Seal the Vectors:
   
```Bash
./tools/vector_sealer/vector_sealer [seal-ref|seal-query] output_file input_file
```

Example:

```Bash
./tools/vector_sealer/vector_sealer seal-ref tools/sealed_data/reference_vectors.dat reference_vectors.txt
   ```

```Bash
./tools/vector_sealer/vector_sealer seal-query tools/sealed_data/query_vector.dat query_vector.txt
```

3. Run Cosine Similarity Computation:

```Bash
./sgx_cos_similarity
```
