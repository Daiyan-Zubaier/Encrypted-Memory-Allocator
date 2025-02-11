#include <assert.h>
#include <unistd.h>
#include <list>
#include <iostream>


//uintptr_t is our system architecture's word size

struct MemBlock{
    //Info about memory block

    //OBJECT HEADER
    std::size_t header; //Check size of block
    MemBlock *next;
    //Payload Pointer
    uintptr_t data[1];
    
    

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

#define XOR_KEY 0xAA                        // Used for encrypting data
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
uintptr_t *alloc(std::size_t size);
void free(uintptr_t *data);
void resetHeap();
MemBlock *find_block(std::size_t size); 
MemBlock *get_header(uintptr_t *data);
MemBlock *coalesce (MemBlock *block);
MemBlock *first_fit(std::size_t size); 
MemBlock *next_fit(std::size_t size);
MemBlock *best_fit(std::size_t size);
MemBlock *exp_free_list(std::size_t size);
MemBlock *seg_list(std::size_t size);
MemBlock *request_from_OS(std::size_t size);
MemBlock *split(MemBlock *block, std::size_t size);
MemBlock *list_allocate(MemBlock *block, std::size_t size);
void xor_encrypt_decrypt(uintptr_t *data, std::size_t size);
//void print_blocks();
int main();


void init(SearchMode mode) {
  search_mode = mode;
  resetHeap();
}

//accessor methods
inline bool is_used(MemBlock *block){
  return block->header & 1; 
}

inline void set_used(MemBlock *block, const bool value){
  block->header = (value == 0) ? (block->header & ~1UL) : (block->header | 1UL); 
}

inline std::size_t get_size(MemBlock *block){
  //Does & with header and 11...1.1..10 
  return block->header & ~1UL;
}

inline void set_size(MemBlock *block, std::size_t value){
  block->header = (block->header & 1UL) | (value & ~1UL);
}

//Does memory allignment
inline std::size_t allign(std::size_t org_size){
  return (org_size + sizeof(uintptr_t) - 1) & ~(sizeof(uintptr_t) - 1);
    //return org_size + sizeof(uintptr_t) - org_size % sizeof(uintptr_t);
}

/*
Allocator
*/


MemBlock *get_header(uintptr_t *data) {
  return (MemBlock *)((char *)data + sizeof(intptr_t) - sizeof(MemBlock));
}

inline std::size_t alloc_size(size_t size) {
  // Total size of memory block being requested minus the first slot size (data[1])
  return size + sizeof(MemBlock) - sizeof(uintptr_t);
}


/*
FREE
------------
-> Writes to program that the memory block that stores the data is unused
*/
// void free(uintptr_t *data){
//   MemBlock *block = get_header(data);
//   xor_encrypt_decrypt(block->data, get_size(block)); 
 
//   if (searchMode != SearchMode::SegregatedList && block->next && !is_used(block->next)){
//     std::cout << "Im in" << std::endl;
//     block = coalesce(block);
//   }
//   set_used(block, false);

//   if (search_mode == SearchMode::FreeList) {
//     free_list.push_back(block);
//   }
// }
void free(uintptr_t *data) {
  MemBlock *block = get_header(data);
  std::cout << "[free] Block at: " << block 
            << " | Size: " << get_size(block) 
            << " | Used: " << is_used(block) 
            << " | Next: " << block->next << std::endl;

  if (block->next) {
      std::cout << "[free] Next Block at: " << block->next 
                << " | Size: " << get_size(block->next) 
                << " | Used: " << is_used(block->next) << std::endl;
  } else {
      std::cout << "[free] No next block (block->next is nullptr)." << std::endl;
  }

  xor_encrypt_decrypt(block->data, get_size(block)); 

  if (block->next && !is_used(block->next)) {
      std::cout << "Im in (Coalescing with next block!)" << std::endl;
      block = coalesce(block);
  } else {
      std::cout << "[free] Not coalescing: ";
      if (!block->next) std::cout << "block->next is null.";
      else if (is_used(block->next)) std::cout << "Next block is marked as used.";
      std::cout << std::endl;
  }

  set_used(block, false);

  if (search_mode == SearchMode::FreeList) {
      free_list.push_back(block);
  }
}


/*
Coalescing, when freeing a block, check for adjacent free blocks
  If adjacent free block, combine current block with free block

This means, we won't have adjacent fragmented free blocks
*/
MemBlock *coalesce(MemBlock *block){
  if (!is_used(block->next)){
    if (block->next == heap_end){
      heap_end = block;
    }
    std::size_t new_size = get_size(block) + get_size(block->next);
    set_size(block, new_size);
    block->next = block->next->next;
  }
  
  return block;
}

/*
Memory Algorithm Allocation Methods
------------------------------------
*/
MemBlock *find_block(std::size_t size) {
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
    //xor_encrypt_decrypt(curr->data, get_size(curr));
    
    if (!is_used(curr) && get_size(curr) >= size){
      
      return list_allocate(curr, size);
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
    if(!is_used(curr) && get_size(curr) >= size){
      last_block = curr;
      return list_allocate(curr, size);
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
    if(!is_used(curr) && get_size(curr) >= size){
      if (size < min){
        min = size;
        min_block = curr;
      }
    }
  }

  return list_allocate(min_block, size);
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

MemBlock *seg_list(std::size_t size){
  std::size_t size_group = size / sizeof(uintptr_t) - 1;   // - 1 for 0-based indexing
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
 
// void xor_encrypt_decrypt(uintptr_t *data, std::size_t size) {
//   for (size_t i = 0; i < size; i++) {
//       data[i] ^= XOR_KEY;
//   }
// }
void xor_encrypt_decrypt(uintptr_t *data, std::size_t size) {
  // Calculate the number of uintptr_t elements
  std::size_t count = size / sizeof(uintptr_t);
  for (size_t i = 0; i < count; i++) {
      data[i] ^= XOR_KEY;
  }
}

/**
 * Initializes the heap, and the search mode.
 */

uintptr_t *alloc(std::size_t size){
  std::cout << "\n\nTHe size is: " << size << std::endl;
  size = allign(size);
  
  //Search for free block: 
  
  if (MemBlock *block = find_block(size)){
    xor_encrypt_decrypt(block->data, size);     
    return block->data;
  }
  
  //Expand heap, if there is no more space in heap
  
  MemBlock *block = request_from_OS(size);

  set_size(block, size);
  set_used(block, true);

  if (search_mode == SearchMode::SegregatedList){
    
    std::size_t size_group = size / sizeof(uintptr_t) - 1;

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

  xor_encrypt_decrypt(block->data, size); 

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
  block->next = nullptr;
  return block;
}

//Splits memory block
MemBlock *split(MemBlock *block, std::size_t size){
  
  std::size_t rem_size{get_size(block) - alloc_size(size)};

  //Gives you address to split point
  std::byte *ptr{reinterpret_cast<std::byte*>(block) + alloc_size(size)};
  MemBlock *remain = reinterpret_cast<MemBlock*>(ptr);

  //Updating the free portion of split
  set_size(remain, rem_size);
  set_used(remain, false);
  remain->next = block->next;

  //Updating the now occupied portion of the split
  set_size(block, size); 
  set_used(block, true);
  block->next = remain;

  return block; 
}


//Allocates with the assumption: there exists space in heap
MemBlock *list_allocate(MemBlock *block, std::size_t size){
  //If block can be split, i.e. is the block size bigger than object header + size
  if (alloc_size(get_size(block)) >= sizeof(MemBlock) + size){
    return split(block, size);
  }
  
  set_used(block, true);
  set_size(block, size);
  
  return block;
}

//To visualize heap
void print_heap() {
  std::cout << "\n----- Heap Dump -----\n";
  for (MemBlock *curr = heap_start; curr != nullptr; curr = curr->next) {
      std::cout << "Block at " << curr
                << " | Size: " << get_size(curr)
                << " | Used: " << (is_used(curr) ? "Yes" : "No")
                << " | Next: " << curr->next
                << std::endl;
  }
  std::cout << "---------------------\n";
}

//Test cases move to test.cpp later
int main(){
  std::cout << "=======================================================\n";
  std::cout << "# First-fit search\n\n";
  

  // --------------------------------------
  // Test case 1: Alignment
  //
  // A request for 3 bytes is aligned to 8.
  //

  auto p1 = alloc(3);
  auto p1b = get_header(p1);

  assert(get_size(p1b) == sizeof(uintptr_t));
  free(p1);

  //// print_blocks()();

  // --------------------------------------
  // Test case 2: Exact amount of aligned bytes
  //

  auto p2 = alloc(8);
  auto p2b = get_header(p2);
  assert(get_size(p2b) == 8);
  

 // // print_blocks()();

  // --------------------------------------
  // Test case 3: Free the object
  //
  // The pointer to the reclaimed block is added
  // to the free list.
  //

  free(p2);
  assert(is_used(p2b) == false);

 // // print_blocks()();

  // --------------------------------------
  // Test case 4: The block is reused
  //
  // A consequent allocation of the same size reuses
  // the previously freed block.
  //

  auto p3 = alloc(8);
  auto p3b = get_header(p3);
  assert(get_size(p3b) == 8);
  assert(p3b == p2b);  // Reused!
  free(p3);


  auto p4 = alloc(8);
  auto p4b = get_header(p4);
  assert(get_size(p4b) == 8);

  auto p5 = alloc(8);
  assert(get_size(get_header(p5)) == 8);
  

  free(p5);
  free(p4);
  // Only one free block with size 16.
  assert(get_size(get_header(p4)) == 16);
  
  print_heap();
  auto p6 = alloc(16);
  auto p6b = get_header(p6);
  assert(p6b == p4b);  // Reused!
  assert(get_size(p6b) == 16);
  

  print_heap();
  auto p7 = alloc(128);
  
  auto p7b = get_header(p7);
  assert(get_size(p7b)== 128);
  
//  // print_blocks()();

  free(p7);

//  // print_blocks()();
  
std::cout << "\nSeg fault here? " << std::endl;
  print_heap();
  auto p8 = alloc(8);
  auto p8b = get_header(p8);
  assert(p8b == p7b);
  assert(get_size(p8b) == 8);

  std::cout << "ALL FIRST FIT TEST CASES PASSED" << std::endl;

//  // print_blocks()();
/*
  // ===========================================================================
  // Next-fit search

  std::cout << "\n=======================================================\n";
  std::cout << "# Next-fit search\n\n";

  init(SearchMode::NextFit);

  // --------------------------------------
  // Test case 5: Next search start position
  //

  // [[8, 1], [8, 1], [8, 1]]
  alloc(8);
  alloc(8);
  alloc(8);
  // print_blocks()();

  // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 1]]
  auto o1 = alloc(16);
  auto o2 = alloc(16);
  // print_blocks()();

  // [[8, 1], [8, 1], [8, 1], [16, 0], [16, 0]]
  free(o1);
  free(o2);
  // print_blocks()();

  // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 0]]
  auto o3 = alloc(16);
  // print_blocks()();

  // Start position from o3:
  assert(heap_end == get_header(o3));

  // [[8, 1], [8, 1], [8, 1], [16, 1], [16, 1]]
  //                           ^ start here
  alloc(16);
  // print_blocks()();

  // ===========================================================================
  // Best-fit search

  std::cout << "\n=======================================================\n";
  std::cout << "# Best-fit search\n\n";

  init(SearchMode::BestFit);

  // --------------------------------------
  // Test case 6: Best-fit search
  //

  // [[8, 1], [64, 1], [8, 1], [16, 1]]
  alloc(8);
  auto z1 = alloc(64);
  alloc(8);
  auto z2 = alloc(16);
  // print_blocks()();

  // Free the last 16
  free(z2);

  // Free 64:
  free(z1);

  // [[8, 1], [64, 0], [8, 1], [16, 0]]
  // print_blocks()();

  // Reuse the last 16 block:
  auto z3 = alloc(16);
  assert(get_header(z3) == get_header(z2));

  // [[8, 1], [64, 0], [8, 1], [16, 1]]
  // print_blocks()();

  // Reuse 64, splitting it to 16, and 24 (considering header)
  z3 = alloc(16);
  assert(get_header(z3) == get_header(z1));

  // [[8, 1], [16, 1], [24, 0], [8, 1], [16, 1]]
  // print_blocks()();

  // ===========================================================================
  // Free-list search

  std::cout << "\n=======================================================\n";
  std::cout << "# Free-list search\n\n";

  init(SearchMode::FreeList);

  alloc(8);
  alloc(8);
  auto v1 = alloc(16);
  alloc(8);
  // print_blocks()();

  free(v1);
  assert(free_list.size() == 1);
  // print_blocks()();

  auto v2 = alloc(16);
  assert(free_list.size() == 0);
  assert(get_header(v1) == get_header(v2));
  // print_blocks()();

  // ===========================================================================
  // Segregated-fit search

  std::cout << "\n=======================================================\n";
  std::cout << "# Segregated-list search\n\n";

  init(SearchMode::SegregatedList);

  auto s1 = alloc(3);
  auto s2 = alloc(8);

  assert(get_header(s1) == segregated_starts[0]);
  assert(get_header(s2) == segregated_starts[0]->next);

  // print_blocks()();

  auto s3 = alloc(16);
  assert(get_header(s3) == segregated_starts[1]);
  // print_blocks()();

  auto s4 = alloc(8);
  assert(get_header(s4) == segregated_starts[0]->next->next);
  // print_blocks()();

  auto s5 = alloc(32);
  assert(get_header(s5) == segregated_starts[3]);
  // print_blocks()();

  free(s1);
  free(s2);
  free(s3);

  // print_blocks()();

  puts("\nAll assertions passed!\n");
*/
  return 0;
}