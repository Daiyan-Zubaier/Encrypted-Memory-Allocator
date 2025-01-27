#include <assert.h>
#include <unistd.h>
#include <list>
#include <iostream>
#include <windows.h>


//intptr_t is our system architecture's word size


struct MemBlock{
    //Info about memory block

    //OBJECT HEADER
    std::size_t size; //Check size of block
    bool used; //Check if it is used
    MemBlock *next; 

    //Data Marker
    intptr_t data[1];
    
    /*
    How memory looks in allocation:
    | Object Header | data[1] (1 byte) | User Data (variable size) | Buffer (for allignment) |
    the slot for data[1] is occupied by the user data
    */
};

//Start and end of the heap
static MemBlock *heap_start = nullptr;
static MemBlock *recent_block = heap_start;


//sbrk implementation for windows:
void *sbrk(intptr_t increment) {
    static char *heap_end = nullptr;
    static char *prev_end = nullptr;

    if (heap_end == nullptr) {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        heap_end = (char *)VirtualAlloc(nullptr, sys_info.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        prev_end = heap_end;
    }

    if (increment != 0) {
        prev_end = heap_end;
        heap_end += increment;
    }

    return prev_end;
}

//Does memory allignment
inline std::size_t allign(std::size_t org_size){
    return org_size + sizeof(intptr_t) - org_size % sizeof(intptr_t);
}

/*
Allocator
*/
intptr_t *alloc(std::size_t size){
  size = allign(size);

  //Search for free block: 
  if (MemBlock *block = find_block(size)){
    return block->data;
  }
  MemBlock *block = request_from_OS(size);

  block->size = size;
  block->used = true;

  //If heap has nothing, initialize it with the ptr that points to the first memory block
  if (heap_start == nullptr){
    heap_start = block;
  }

  if (recent_block != nullptr){
    recent_block->next = block;
  }

  recent_block = block;

  //Gives us the mem location of the first slot to start from
  return block->data; 
  
}

MemBlock *get_header(intptr_t *data) {
  return (MemBlock *)((char *)data + sizeof(intptr_t) -
                   sizeof(MemBlock));
}

inline size_t alloc_size(size_t size) {
  // Total size of memory block being requested minus the first slot size (data[1])
  return size + sizeof(MemBlock) - sizeof(intptr_t);
}

MemBlock *request_from_OS(size_t size) {
  // Current heap break
  MemBlock *block = (MemBlock *)sbrk(0);           
  
  //Out of memory
  if (sbrk(alloc_size(size)) == (void *)-1) {   
    return nullptr;
  }
 
  return block;
}

/*
FREE
------------
-> Writes to program that the memory block that stores the data is unused
*/
void free(intptr_t *data){
  MemBlock *block = get_header(data);
  block->used = false;
}

/*
Memory Algorithm Allocation Methods
------------------------------------
*/
MemBlock *findBlock(std::size_t size) {
  return first_fit(size);
}

/*
FIRST FIT - 
Go through memory, find me the FIRST piece of memory unallocated, 
*/
MemBlock *first_fit(std::size_t size){
  for (MemBlock *curr = heap_start; curr != nullptr; curr = curr->next){
    if (!curr->used && curr->size > size){
      return curr;
    }
  }

  return nullptr;
}

/*
NEXT FIT - 
Same as first algo, but remember where we ended our previous allocation
*/
MemBlock *next_fit(std::size_t size){
  
}





enum class SearchMode {
  FirstFit,
  NextFit,
};
 
/**
 * Previously found block. Updated in `nextFit`.
 */
static MemBlock *last_block = heap_start; 

/**
 * Current search mode.
 */
static auto searchMode = SearchMode::FirstFit;
 
/**
 * Reset the heap to the original position.
 */
void resetHeap() {
  // Already reset.
  if (heap_start == nullptr) {
    return;
  }
 
  brk(heap_start);
 
  heap_start = nullptr;
  recent_block = nullptr;
  last_block = nullptr;
}
 
/**
 * Initializes the heap, and the search mode.
 */
void init(SearchMode mode) {
  searchMode = mode;
  resetHeap();
}