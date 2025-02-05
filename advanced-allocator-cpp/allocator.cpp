#include <assert.h>
#include <unistd.h>
#include <list>
#include <iostream>


//intptr_t is our system architecture's word size

struct MemBlock{
    //Info about memory block

    //OBJECT HEADER
    std::size_t header; //Check size of block

    //Payload Pointer
    intptr_t data[1];
    
    MemBlock *next;
    //Could add a footer 

    /*
    How memory looks in allocation:
    | Object Header | data[1] (1 byte) | User Data (variable size) | Buffer (for allignment) |
    the slot for data[1] is occupied by the user data
    */
};


enum class SearchMode {
  FirstFit,
  NextFit,
  BestFit,
  FreeList,
  SegregatedList
};

static auto search_mode = SearchMode::FirstFit;

static MemBlock *heap_start = nullptr;      // Start of heap
static MemBlock *heap_end = heap_start;     // End of heap 
static MemBlock *last_block = heap_start;   // Last Successfully allocated block
static std::list<MemBlock*> free_list;      // Tracks all free blocks

//Segregrated lists data structures
MemBlock *segregated_starts[] = {
  nullptr,    // 8
  nullptr,    // 16
  nullptr,    // 32
  nullptr,    // 64
  nullptr,    // 128
  nullptr,    // any > 128
};
MemBlock *segregated_ends[] = {
  nullptr,    // 8
  nullptr,    // 16
  nullptr,    // 32
  nullptr,    // 64
  nullptr,    // 128
  nullptr,    // any > 128
};

//Function Declarations
void init(SearchMode mode);
intptr_t *alloc(std::size_t size);
void free(intptr_t *data);
void resetHeap();
MemBlock *find_block(size_t size); 
MemBlock *get_header(intptr_t *data);
MemBlock *coalesce (MemBlock *block);
MemBlock *get_header(intptr_t *data); 
MemBlock *first_fit(std::size_t size); 
MemBlock *next_fit(std::size_t size);
MemBlock *best_fit(std::size_t size);
MemBlock *request_from_OS(size_t size);
MemBlock *split(MemBlock *block, std::size_t size);
MemBlock *list_allocate(MemBlock *block, std::size_t size);
int main();


void init(SearchMode mode) {
  search_mode = mode;
  resetHeap();
}

inline bool is_used(MemBlock *block){
  return block->header & 1; 
}

inline std::size_t get_size(MemBlock *block){
  //Does & with header and 11...1...10 
  return block->header & ~1L;
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

  if (block->next && !block->next->used){
    block = coalesce(block);
  }
  is_used(block) = false;

  if (search_mode == SearchMode::FreeList) {
    free_list.push_back(block);
  }
}


/*
Coalescing, when freeing a block, check for adjacent free blocks
  If adjacent free block, combine current block with free block

This means, we won't have adjacent fragmented free blocks
*/
MemBlock *coalesce (MemBlock *block){
  get_size(block) += block->next->size;
  block->next = block->next->next;
  return block;
}

/*
Memory Algorithm Allocation Methods
------------------------------------
*/
MemBlock *find_block(size_t size) {
  switch (search_mode) {
    case SearchMode::FirstFit:
      return first_fit(size);
    case SearchMode::NextFit:
      return next_fit(size);
    case SearchMode::BestFit:
      return best_fit(size);
    case SearchMode::FreeList:
      return exp_free_list(size);
    case SearchMode::SegregatedList:
      return seg_list(size);
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

/*
Explicit Free List Algorithm
  Tracks all free blocks in a list, makes allocation FAST
*/
MemBlock *exp_free_list(std::size_t size) {
  for (const auto &block : free_list) {
    if (get_size(block) < size) {
      continue;
    }
    free_list.remove(block);
    return list_allocate(block, size);
  }
  return nullptr;
}

/*
Segregated Lists:
  Have predetermined sized lists, and allocate blocks, based on the list they best fit in
*/

MemBlock *segregated_fit(std::size_t size){
  std::size_t size_group = size / sizeof(intptr_t) - 1;   // - 1 for 0-based indexing
  MemBlock *og_heap_start = heap_start; 
  
  heap_start = segregated_starts[size_group];
  
  //Now the heap start is the heap start of a section of the segregated list
  MemBlock *block = first_fit(size);

  //Restoring status quo
  heap_start = og_heap_start; 
  return block;
}
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
  heap_end = nullptr;
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

  //Expand heap, if there is no more space in heap
  MemBlock *block = request_from_OS(size);

  get_size(block) = size;
  is_used(block) = true;

  if (search_mode == SearchMode::SegregatedList){
    std::size_t size_group = size / sizeof(intptr_t) - 1;

    if (segregated_starts[size_group]){
      segregated_starts[size_group] = block;
    }
    // Chain the blocks in the bucket.
    if (segregated_ends[size_group] != nullptr) {
      segregated_ends[size_group]->next = block;
    }
    segregated_ends[size_group] = block;
  }
  else{
    //If heap has nothing, initialize it with the ptr that points to the first memory block
    if (heap_start == nullptr){
      heap_start = block;
    }

    if (heap_end != nullptr){
      heap_end->next = block;
    }

    heap_end = block;
  }
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
  std::size_t const hdr_size = sizeof(MemBlock);
  std::size_t rem_size = get_size(block) - alloc_size(size) - hdr_size;

  std::byte *ptr{reinterpret_cast<std::byte*>(block->next) + rem_size + hdr_size};
  MemBlock *remain = reinterpret_cast<MemBlock*>(ptr);

  //Updating the free portion of the split
  remain->size = rem_size;
  remain->used = false;
  remain->next = block->next;

  //Updating the now occupied portion of the split
  get_size(block) = size; 
  is_used(block) = true;
  block->next = remain;
  
  
  return block; 
}

//Allocates with the assumption: there exists space in heap
MemBlock *list_allocate(MemBlock *block, std::size_t size){
  //If block can be split, i.e. is the block size bigger than object header + size
  if (alloc_size(get_size(block)) >= sizeof(MemBlock) + size){
    return split(block, size);
  }
  is_used(block) = true;
  return block;
}

int main(){
  // --------------------------------------
  // -------------------------------------
  // Test case 1: Alignment
  //
  // A request for 3 bytes is aligned to 8.
  //
 
  auto p1 = alloc(3);                        // (1)
  auto p1b = get_header(p1);
  assert(is_used(p1b) == sizeof(intptr_t));
 
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
  assert(is_used(p2b) == false);

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