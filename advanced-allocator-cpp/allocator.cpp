#include <cstdint>

//intptr_t is our system architecture's word size

struct MemBlock{
    //Info about memory block
    std::size_t size; //Check size of block
    bool used; //Check if it is used
    
    //Not necessary, might remove later
    MemBlock *next; 

    //Data
    //Beginning of data
    intptr_t data[1];
};

//Does memory allignment
inline std::size_t allign(std::size_t org_size){
    return org_size + sizeof(intptr_t) - org_size % sizeof(intptr_t);
}

//Allocator
intptr_t *alloc(std::size_t size){
    allign(size);
}

