#include <cstdint>
#include <unistd.h> 
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
static MemBlock *heap_end = heap_start;


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

//Allocator
intptr_t *alloc(std::size_t size){
  size = allign(size);

  MemBlock *block = request_from_OS(size);

  block->size = size;
  block->used = true;


  
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
