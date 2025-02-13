# Custom Memory Allocator

## Overview
This project implements a custom dynamic memory allocator in C++ with multiple allocation strategies, including **First Fit, Next Fit, Best Fit, Free List, and Segregated List**. It features memory alignment, coalescing, block splitting, and an **XOR-based encryption mechanism** for added security.

## Features
- **Multiple Allocation Strategies**: First Fit, Next Fit, Best Fit, Free List, Segregated List.
- **Memory Management**: Coalescing adjacent free blocks, explicit free list, and segregated lists for efficiency.
- **Custom Heap Management**: Uses `sbrk()` to request memory from the OS.
- **XOR Encryption**: Encrypts/decrypts allocated data to prevent unintended access.
- **Heap Visualization**: Print function to debug memory layout.

## File Structure
- **`allocator.cpp`**: Core implementation of the memory allocator.
- **`allocator.h`**: Header file defining memory block structure and function prototypes.
- **`alloc_test.cpp`**: Test cases for memory allocation and deallocation.

## Usage

### Initialization
Before allocation, initialize the allocator with a specific search mode:
```cpp
init(SearchMode::FirstFit);
```

## Memory Allocation
Allocate memory using:
```cpp
uintptr_t *ptr = alloc(64);
```

## Memory Deallocation
Free allocated memory:
```cpp
free(ptr);
```

## Heap Visualization
To inspect the heap layout:
```cpp
print_heap();
```

## Search Modes

| Mode              | Description                                                              |
|-------------------|--------------------------------------------------------------------------|
| First Fit         | Finds the first available block that fits the requested size.          |
| Next Fit          | Similar to First Fit but continues searching from the last allocated block. |
| Best Fit          | Finds the smallest available block that fits the requested size.       |
| Free List         | Uses an explicit free list to track free memory.                       |
| Segregated List   | Uses separate lists for different block sizes.  


## XOR Encryption
Data stored in allocated blocks is XOR encrypted using a predefined key (0xAA). Encryption/decryption is done in place.

Note: Only supported for linux terminal, MacOS and Windows support coming soon. 
