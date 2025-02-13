#include <assert.h>
#include <unistd.h>
#include <list>
#include <iostream>
#include "allocator.h"

//Test cases move to test.cpp later
// ---------------------------------------------------------------------------
// XOR Encryption Tests
// ---------------------------------------------------------------------------
void testXorEncryption() {
  std::cout << "=======================================================\n";
  std::cout << "# XOR Encryption Test\n\n";

  const std::size_t numElements = 4;
  // Create an array with known values.
  uintptr_t original[numElements] = { 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0x0 };
  uintptr_t buffer[numElements];

  // Copy original data into buffer.
  for (std::size_t i = 0; i < numElements; i++) {
      buffer[i] = original[i];
  }

  // Encrypt the buffer.
  xor_encrypt_decrypt(buffer, numElements * sizeof(uintptr_t));

  // Verify that the data has changed.
  bool changed = false;
  for (std::size_t i = 0; i < numElements; i++) {
      if (buffer[i] != original[i]) {
          changed = true;
          break;
      }
  }
  assert(changed && "Data should change after encryption.");

  // Decrypt (encrypt again with XOR) to restore original data.
  xor_encrypt_decrypt(buffer, numElements * sizeof(uintptr_t));
  for (std::size_t i = 0; i < numElements; i++) {
      assert(buffer[i] == original[i] && "Decrypted data does not match original.");
  }
  std::cout << "XOR Encryption tests passed.\n\n";
}

// ---------------------------------------------------------------------------
// First-Fit Search Tests
// ---------------------------------------------------------------------------
void testFirstFit() {
  std::cout << "=======================================================\n";
  std::cout << "# First-fit search\n\n";

  // Test case 1: Alignment
  auto p1 = alloc(3);
  auto p1b = get_header(p1);
  assert(get_size(p1b) == sizeof(uintptr_t));
  free(p1);

  // Test case 2: Exact amount of aligned bytes
  auto p2 = alloc(8);
  auto p2b = get_header(p2);
  assert(get_size(p2b) == 8);

  // Test case 3: Free the object and verify it's marked free.
  free(p2);
  assert(!is_used(p2b));

  // Test case 4: Block reuse.
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
  // Now there should be one free block of size 16.
  assert(get_size(get_header(p4)) == 16);

  print_heap();
  auto p6 = alloc(16);
  auto p6b = get_header(p6);
  assert(p6b == p4b);  // Reused!
  assert(get_size(p6b) == 16);

  print_heap();
  auto p7 = alloc(128);
  auto p7b = get_header(p7);
  assert(get_size(p7b) == 128);

  free(p7);
  print_heap();
  auto p8 = alloc(8);
  auto p8b = get_header(p8);
  assert(p8b == p7b);
  assert(get_size(p8b) == 8);

  std::cout << "ALL FIRST FIT TEST CASES PASSED\n";
  print_heap();
}

// ---------------------------------------------------------------------------
// Next-Fit Search Tests
// ---------------------------------------------------------------------------
void testNextFit() {
  std::cout << "\n=======================================================\n";
  std::cout << "# Next-fit search\n\n";

  init(SearchMode::NextFit);

  // Create several blocks.
  alloc(8);
  alloc(8);
  alloc(8);
  print_heap();

  auto o1 = alloc(16);
  auto o2 = alloc(16);
  free(o1);
  free(o2);

  auto o3 = alloc(16);
  // Check that heap_end points to the block allocated.
  assert(heap_end == get_header(o3));

  // Allocate another block.
  alloc(16);

  std::cout << "ALL NEXT FIT TEST CASES PASSED\n";
}

// ---------------------------------------------------------------------------
// Best-Fit Search Tests
// ---------------------------------------------------------------------------
void testBestFit() {
  std::cout << "\n=======================================================\n";
  std::cout << "# Best-fit search\n\n";

  init(SearchMode::BestFit);

  alloc(8);
  auto z1 = alloc(64);
  alloc(8);
  auto z2 = alloc(16);
  print_heap();

  // Free some blocks.
  free(z2);
  free(z1);

  print_heap();
  auto z3 = alloc(16);
  print_heap();
  // The best-fit algorithm should reuse the block freed for z2.
  assert(get_header(z3) == get_header(z2));

  // Allocate again expecting a split (using the block from z1).
  z3 = alloc(16);
  assert(get_header(z3) == get_header(z1));

  std::cout << "All best-fit assertions passed\n";
}

// ---------------------------------------------------------------------------
// Free-List Search Tests
// ---------------------------------------------------------------------------
void testFreeList() {
  std::cout << "\n=======================================================\n";
  std::cout << "# Free-list search\n\n";

  init(SearchMode::FreeList);

  alloc(8);
  alloc(8);
  auto v1 = alloc(16);
  alloc(8);

  free(v1);
  assert(free_list.size() == 1);

  auto v2 = alloc(16);
  assert(free_list.size() == 0);
  assert(get_header(v1) == get_header(v2));

  std::cout << "All free list search assertions passed\n";
}

// ---------------------------------------------------------------------------
// Segregated-List Search Tests
// ---------------------------------------------------------------------------
void testSegregatedList() {
  std::cout << "\n=======================================================\n";
  std::cout << "# Segregated-list search\n\n";

  init(SearchMode::SegregatedList);
  print_heap();

  auto s1 = alloc(3);
  auto s2 = alloc(8);
  print_heap();

  // Assuming that the bucket for sizes ≤8 is at index 0.
  assert(get_header(s1) == segregated_starts[0]);
  assert(get_header(s2) == segregated_starts[0]->next);

  auto s3 = alloc(16);
  // Assuming bucket 1 is used for sizes ≤16.
  assert(get_header(s3) == segregated_starts[1]);

  auto s4 = alloc(8);
  // Check that s4 is chained after s1 and s2.
  assert(get_header(s4) == segregated_starts[0]->next->next);

  auto s5 = alloc(32);
  // Assuming bucket 3 is used for sizes ≤32.
  assert(get_header(s5) == segregated_starts[3]);

  free(s1);
  free(s2);
  free(s3);

  std::cout << "All segregated-list search assertions passed\n";
}

// ---------------------------------------------------------------------------
// Main Test Runner
// ---------------------------------------------------------------------------
int main() {
  testXorEncryption();
  testFirstFit();
  testNextFit();
  testBestFit();
  testFreeList();
  testSegregatedList();

  std::cout << "\nAll assertions passed!\n";
  return 0;
}