#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// XOR Encryption key 
#define XOR_KEY 0xAA

// Structure for memory block
typedef struct MemoryBlock {
    size_t size;
    struct MemoryBlock *next;
    unsigned char data[];
} MemoryBlock;

// Head of the free list
static MemoryBlock *free_list = NULL;

// Function to encrypt/decrypt data
void xor_encrypt_decrypt(unsigned char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        data[i] ^= XOR_KEY;
    }
}

// Custom malloc function with XOR encryption
void* xmalloc(size_t size) {
    // Check if free_list has a memory block to use
    MemoryBlock *block = free_list;
    if (block) {
        free_list = block->next;  // Remove block from free list
    } else {
        block = (MemoryBlock*) malloc(sizeof(MemoryBlock) + size);
        if (!block) {
            return NULL; // Allocation failed
        }
        block->size = size;
    }

    // Encrypt the allocated memory block
    xor_encrypt_decrypt(block->data, size);

    return block->data;
}

// Custom free function with XOR decryption
void xfree(void *ptr) {
    if (!ptr) return;

    MemoryBlock *block = (MemoryBlock*)((unsigned char*)ptr - sizeof(MemoryBlock));

    // Decrypt the memory block before freeing it
    xor_encrypt_decrypt(block->data, block->size);

    // Add the block to the free list
    block->next = free_list;
    free_list = block;
}

int main() {
    // Example usage of the custom allocator
    char *data = (char*) xmalloc(20);
    if (data) {
        strcpy(data, "Encrypted Data!");
        printf("Allocated data: %s\n", data);
    } else {
        printf("Memory allocation failed.\n");
    }

    // Free the allocated memory
    xfree(data);

    return 0;
}
