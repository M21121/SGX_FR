# SGX Secure Number Comparison System

A secure number comparison system using Intel SGX (Software Guard Extensions) that performs efficient binary search operations within an enclave.

## Overview

This system implements a secure number comparison mechanism using Intel SGX technology. It allows for secure storage and comparison of large sets of numbers (up to 33,554,432 values) within an SGX enclave, providing memory protection and secure computation guarantees.

## Features

- Secure number comparison using SGX enclaves
- Support for up to 33,554,432 (2²⁵) unique values
- Binary search implementation for efficient lookups
- Performance metrics tracking including:
  - EENTER/EEXIT calls
  - Execution time
  - Page faults
  - Statistical analysis
- Automated test file generation
- Secure data sealing capabilities

## Directory Structure

Text Only

`. ├── App/ # Application code ├── common/ # Shared type definitions ├── Enclave/ # SGX enclave implementation ├── tools/ # Utility tools │ └── value_sealer/ # Data sealing tool └── scripts/ # Helper scripts`

## Prerequisites

- Intel SGX SDK
- Intel SGX PSW (Platform Software)
- C++ compiler with C++11 support
- Linux environment

## Building

1. Build the main application:
  
  Bash
  
  `make`
  
2. Build the value sealer tool:
  
  Bash
  
  `cd tools/value_sealer make`
  

## Usage

### 1. Generate Test Files

Bash

`./generate_test_files.sh <number_of_files>`

### 2. Run the Application

Bash

`./sgx_equality_test`

### 3. Using the Value Sealer

Bash

`./value_sealer seal output.dat input.txt # Seal secret values ./value_sealer seal-tests test.dat test.txt # Seal test values`

## Performance Metrics

The system tracks and reports various performance metrics:

- Total EENTER/EEXIT calls
- Execution time statistics (min, mean, median, max)
- Page fault counts
- Per-operation timing

## Security Features

- Secure data storage within SGX enclave
- Protected memory access
- Sealed data storage
- Binary search implementation to minimize timing attacks
- Page fault tracking for security analysis

## Limitations

- Maximum of 33,554,432 unique values
- Requires SGX-capable hardware
- Linux-only implementation
