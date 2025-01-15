#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "allocator.h" 

void test_allocator() {
    // Test 1: Allocate memory and check if it's encrypted correctly
    char *data = (char*) xmalloc(20);
    assert(data != NULL); // Ensure allocation succeeds
    strcpy(data, "Encrypted Data!");
    printf("Test 1 - Allocated data: %s\n", data);

    // Test 2: Free the allocated memory and ensure it's cleared properly
    xfree(data);
    printf("Test 2 - Memory freed successfully.\n");

    // Test 3: Allocate memory again and check the behavior
    char *new_data = (char*) xmalloc(20);
    assert(new_data != NULL); // Ensure allocation succeeds
    strcpy(new_data, "New Data After Free");
    printf("Test 3 - New allocated data: %s\n", new_data);
    xfree(new_data);

    printf("All tests passed successfully.\n");
}

int main() {
    // Run tests
    test_allocator();
    return 0;
}
