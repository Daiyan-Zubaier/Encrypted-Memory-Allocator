#include <cstdint>

struct MemBlock{
    //Info about memory block
    std::size_t size; //Check size of block
    bool used; //Check if it is used
    
    //Not necessary, might remove later
    MemBlock *next; 

    //Data
    //Beginning of data
    intptr_t data[1];

    
}