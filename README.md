# Encrypted Memory Allocator

This project implements a custom memory allocator in C with XOR encryption to enhance the security of dynamic memory management. The allocator provides functions similar to `malloc` and `free`, with added encryption to protect sensitive data in memory.

## Features
- **Custom Memory Allocation**: Implements basic functions for dynamic memory management.
- **XOR Encryption**: Ensures data security by encrypting memory blocks during allocation.
- **Performance Optimization**: Balances security and efficiency through optimized algorithms.

## How to Use
1. **Clone the repository:**
   ```bash
   https://github.com/Daiyan-Zubaier/Encrypted-Memory-Allocator.git```
2. Navigate to the project directory:
   `cd encrypted-memory-allocator`
   
3. Compile the project:
   `gcc allocator.c -o allocator.exe`
   
4. Run the executable:
   `./allocator.exe`
