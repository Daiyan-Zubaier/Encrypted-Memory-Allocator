
#include <unistd.h>
#include <list>
#include <iostream>

#pragma once 

enum class SearchMode {
    FirstFit,
    NextFit,
    BestFit,
    FreeList,
    SegregatedList
};
struct MemBlock{
    std::size_t header; 
    MemBlock *next;
    uintptr_t data[1];
};

extern std::list<MemBlock*> free_list;
extern MemBlock *segregated_starts[];
extern MemBlock *heap_end;

//Function Declarations
uintptr_t *alloc(std::size_t size);
void init(SearchMode mode);
void free(uintptr_t *data);
void resetHeap();
void xor_encrypt_decrypt(uintptr_t *data, std::size_t size);
void print_heap();  
MemBlock *get_header(uintptr_t *data);
bool is_used(MemBlock *block);
void set_used(MemBlock *block, const bool value);                
std::size_t get_size(MemBlock *block);
void set_size(MemBlock *block, std::size_t value);