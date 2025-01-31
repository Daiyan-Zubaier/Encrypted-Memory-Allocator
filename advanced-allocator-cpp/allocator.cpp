#include <assert.h>
#include <unistd.h>
#include <list>
#include <iostream>


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

enum class SearchMode {
  FirstFit,
  NextFit,
  BestFit
};

static auto searchMode = SearchMode::FirstFit;
//Start and end of the heap
static MemBlock *heap_start = nullptr;
static MemBlock *recent_block = heap_start;
/*
Previously found block. Updated in next_fit(). 
*/
static MemBlock *last_block = heap_start; 

//Function Declarations
void init(SearchMode mode);
MemBlock *find_block(size_t size); 
MemBlock *get_header(intptr_t *data);
void free(intptr_t *data);
MemBlock *findBlock(std::size_t size);
MemBlock *get_header(intptr_t *data); 
void free(intptr_t *data); 
MemBlock *findBlock(std::size_t size);
MemBlock *first_fit(std::size_t size); 
MemBlock *next_fit(std::size_t size);
MemBlock *best_fit(std::size_t size);
void resetHeap();
intptr_t *alloc(std::size_t size);
MemBlock *request_from_OS(size_t size);
MemBlock *split(MemBlock *block, std::size_t size);
MemBlock *list_allocate(MemBlock *block, std::size_t size);
int main();


void init(SearchMode mode) {
  searchMode = mode;
  resetHeap();
}

MemBlock *find_block(size_t size) {
  return first_fit(size);
}

//Does memory allignment
inline std::size_t allign(std::size_t org_size){
  return (org_size + sizeof(intptr_t) - 1) & ~(sizeof(intptr_t) - 1);
    //return org_size + sizeof(intptr_t) - org_size % sizeof(intptr_t);
}

/*
Allocator
*/


MemBlock *get_header(intptr_t *data) {
  return (MemBlock *)((char *)data + sizeof(intptr_t) -
                   sizeof(MemBlock));
}

inline size_t alloc_size(size_t size) {
  // Total size of memory block being requested minus the first slot size (data[1])
  return size + sizeof(MemBlock) - sizeof(intptr_t);
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
MemBlock *findBlock(size_t size) {
  switch (searchMode) {
    case SearchMode::FirstFit:
      return first_fit(size);
    case SearchMode::NextFit:
      return next_fit(size);
    case SearchMode::BestFit:
      return best_fit(size);
  }
  return nullptr;
}

/*
FIRST FIT - 
Go through memory, find me the FIRST piece of memory unallocated, 
*/
MemBlock *first_fit(std::size_t size){
  for (MemBlock *curr = heap_start; curr != nullptr; curr = curr->next){
    if (!curr->used && curr->size >= size){
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
  if (last_block == nullptr){
    last_block = heap_start;
  }

  for(MemBlock *curr = last_block; curr != nullptr; curr = curr->next){
    if(!curr->used && curr->size >= size){
      last_block = curr;
      return curr;
    }

    //Circled back
    if (curr->next == last_block) {
      break;
    }
    
    //Let's get it started again
     if (curr->next == nullptr) {
      curr = heap_start; 
    }
  }
  return nullptr;

}


MemBlock *best_fit(std::size_t size){
  std::size_t min {SIZE_MAX};
  MemBlock *min_block {nullptr};

  for(MemBlock *curr = heap_start; curr != nullptr; curr = curr->next){
    if(!curr->used && curr->size >= size){
      if (size < min){
        min = size;
        min_block = curr;
      }
    }
  }

  return min_block;
}



 

/**
 * Current search mode.
 */

 
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

MemBlock *request_from_OS(size_t size) {
  // Current heap break
  MemBlock *block = (MemBlock *)sbrk(0);           
  
  //Out of memory
  if (sbrk(alloc_size(size)) == (void *)-1) {   
    return nullptr;
  }
 
  return block;
}

//Splits memory block
MemBlock *split(MemBlock *block, std::size_t size){
  /*
  We are givne the pointer to the full free block, and the size of the block we want to allocate
   */

  std::size_t rem_size = (block->size > size) ?  (block->size - size):;

  std::byte *ptr{reinterpret_cast<std::byte*>(block->next) + rem_size};
  MemBlock *remain = reinterpret_cast<MemBlock*>(ptr);

  remain->size = rem_size;
  
  //IRemaining block is smaller
  if (rem_size){

  }
  else{

  }
}
//Allocates 
MemBlock *list_allocate(MemBlock *block, std::size_t size){
  
}

int main(){
  // --------------------------------------
  // Test case 1: Alignment
  //
  // A request for 3 bytes is aligned to 8.
  //
 
  auto p1 = alloc(3);                        // (1)
  auto p1b = get_header(p1);
  assert(p1b->size == sizeof(intptr_t));
 
  // --------------------------------------
  // Test case 2: Exact amount of aligned bytes
  //
 
  auto p2 = alloc(8);                        // (2)
  auto p2b = get_header(p2);
  assert(p2b->size == 8);
 
 
  puts("\nAll assertions passed!\n");
  
    // Init the heap, and the searching algorithm.
  init(SearchMode::NextFit);
  
  // --------------------------------------
  // Test case 3: Free the object
  //
  
  free(p2);
  assert(p2b->used == false);

  // --------------------------------------
  // Test case 4: The block is reused
  //
  // A consequent allocation of the same size reuses
  // the previously freed block.
  //
  
  auto p3 = alloc(8);
  auto p3b = get_header(p3);
  assert(p3b->size == 8);
  assert(p3b == p2b);  // Reused!

  // Init the heap, and the searching algorithm.
  init(SearchMode::NextFit);
  
  // --------------------------------------
  // Test case 5: Next search start position
  //
  
  // [[8, 1], [8, 1], [8, 1]]
  alloc(8);
  alloc(8);
  alloc(8);
  
  // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 1]]
  auto o1 = alloc(16);
  auto o2 = alloc(16);
  
  // [[8, 1], [8, 1], [8, 1], [16, 0], [16, 0]]
  free(o1);
  free(o2);
  
  // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 0]]
  auto o3 = alloc(16);
  
  // Start position from o3:
  assert(last_block == get_header(o3));
  
  // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 1]]
  //                           ^ start here
  //alloc(16);
  
  init(SearchMode::BestFit);
 
  // --------------------------------------
  // Test case 6: Best-fit search
  //
  
  // [[8, 1], [64, 1], [8, 1], [16, 1]]
  alloc(8);
  auto z1 = alloc(64);
  alloc(8);
  auto z2 = alloc(16);
  
  // Free the last 16
  free(z2);
  
  // Free 64:
  free(z1);
  
  // [[8, 1], [64, 0], [8, 1], [16, 0]]
  
  // Reuse the last 16 block:
  auto z3 = alloc(16);
  assert(get_header(z3) == get_header(z2));
  
  // [[8, 1], [64, 0], [8, 1], [16, 1]]
  
  // Reuse 64, splitting it to 16, and 24 (considering header)
  z3 = alloc(16);
  assert(get_header(z3) == get_header(z1));
  
  free(z3);
  // [[8, 1], [16, 1], [24, 0], [8, 1], [16, 1]]
}