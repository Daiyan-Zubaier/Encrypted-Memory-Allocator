

int main(int argc, char const *argv[]) {
 
  // --------------------------------------
  // Test case 1: Alignment
  //
  // A request for 3 bytes is aligned to 8.
  //
 
  auto p1 = alloc(3);                        // (1)
  auto p1b = getHeader(p1);
  assert(p1b->size == sizeof(word_t));
 
  // --------------------------------------
  // Test case 2: Exact amount of aligned bytes
  //
 
  auto p2 = alloc(8);                        // (2)
  auto p2b = getHeader(p2);
  assert(p2b->size == 8);
 
 
  puts("\nAll assertions passed!\n");
 
}