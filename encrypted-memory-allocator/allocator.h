#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

// Custom malloc function with XOR encryption
void* xmalloc(size_t size);

// Custom free function with XOR decryption
void xfree(void* ptr);

#endif // ALLOCATOR_H
