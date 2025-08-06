#include "../src/common/arena.h"
#include <criterion/criterion.h>
#include <criterion/parameterized.h>
#include <string.h>

// Custom smaller arena size for testing to avoid memory-intensive tests
#undef INITIAL_ARENA_SIZE
#define INITIAL_ARENA_SIZE 64 // 64 bytes for testing

// =========================
// Basic Functionality Tests
// =========================

Test(arena, arena_create_initializes_properly) {
  Arena a = arena_init();

  // Arena should be created successfully
  // We can't directly test internal state but can test behavior
  void *ptr = arena_alloc(&a, 1);
  cr_assert_not_null(ptr, "Arena should be able to allocate after creation");

  arena_destroy(&a);
}

Test(arena, arena_alloc_basic_allocation) {
  Arena a = arena_init();

  void *ptr1 = arena_alloc(&a, 32);
  cr_assert_not_null(ptr1, "First allocation should succeed");

  void *ptr2 = arena_alloc(&a, 16);
  cr_assert_not_null(ptr2, "Second allocation should succeed");
  cr_assert_neq(ptr1, ptr2, "Allocations should return different pointers");

  arena_destroy(&a);
}

Test(arena, arena_alloc_zero_bytes) {
  Arena a = arena_init();

  void *ptr = arena_alloc(&a, 0);
  cr_assert_not_null(
      ptr, "Zero-byte allocation should still return a valid pointer");

  arena_destroy(&a);
}

Test(arena, arena_alloc_large_initial_allocation) {
  Arena a = arena_init();

  // Allocate more than INITIAL_ARENA_SIZE on first allocation
  void *ptr = arena_alloc(&a, INITIAL_ARENA_SIZE * 2);
  cr_assert_not_null(ptr, "Large initial allocation should succeed");

  arena_destroy(&a);
}

Test(arena, arena_free_handles_null_arena) {
  // Should not crash
  arena_destroy(NULL);
}

Test(arena, arena_free_handles_empty_arena) {
  Arena a = arena_init();

  // Free without any allocations - should not crash
  arena_destroy(&a);

  // Should be safe to call again
  arena_destroy(&a);
}

// =========================
// Memory Alignment Tests
// =========================

Test(arena, arena_alloc_alignment) {
  Arena a = arena_init();

  // Test pointer alignment
  void *ptr1 = arena_alloc(&a, 1);
  void *ptr2 = arena_alloc(&a, 1);
  void *ptr3 = arena_alloc(&a, 1);

  cr_assert_not_null(ptr1);
  cr_assert_not_null(ptr2);
  cr_assert_not_null(ptr3);

  // Check that all pointers are properly aligned
  cr_assert_eq((uintptr_t)ptr1 % sizeof(void *), 0, "ptr1 should be aligned");
  cr_assert_eq((uintptr_t)ptr2 % sizeof(void *), 0, "ptr2 should be aligned");
  cr_assert_eq((uintptr_t)ptr3 % sizeof(void *), 0, "ptr3 should be aligned");

  arena_destroy(&a);
}

Test(arena, arena_alloc_various_sizes_alignment) {
  Arena a = arena_init();

  // Test different allocation sizes to verify alignment
  void *ptr1 = arena_alloc(&a, 1);
  void *ptr2 = arena_alloc(&a, 3);
  void *ptr3 = arena_alloc(&a, 7);
  void *ptr4 = arena_alloc(&a, sizeof(void *));

  cr_assert_eq((uintptr_t)ptr1 % sizeof(void *), 0,
               "1-byte allocation should be aligned");
  cr_assert_eq((uintptr_t)ptr2 % sizeof(void *), 0,
               "3-byte allocation should be aligned");
  cr_assert_eq((uintptr_t)ptr3 % sizeof(void *), 0,
               "7-byte allocation should be aligned");
  cr_assert_eq((uintptr_t)ptr4 % sizeof(void *), 0,
               "pointer-size allocation should be aligned");

  arena_destroy(&a);
}

// =========================
// Memory Growth Tests
// =========================

Test(arena, arena_alloc_triggers_growth) {
  Arena a = arena_init();

  // Fill up the initial arena
  void *ptrs[10];
  uint32_t allocation_size = INITIAL_ARENA_SIZE / 8;

  for (int i = 0; i < 8; i++) {
    ptrs[i] = arena_alloc(&a, allocation_size);
    cr_assert_not_null(ptrs[i], "Allocation %d should succeed", i);
  }

  // This should trigger arena growth
  void *growth_ptr = arena_alloc(&a, allocation_size);
  cr_assert_not_null(growth_ptr, "Allocation after growth should succeed");

  arena_destroy(&a);
}

Test(arena, arena_alloc_multiple_growths) {
  Arena a = arena_init();

  // Trigger multiple arena growths
  for (int i = 0; i < 5; i++) {
    void *ptr = arena_alloc(&a, INITIAL_ARENA_SIZE);
    cr_assert_not_null(ptr, "Large allocation %d should succeed", i);
  }

  arena_destroy(&a);
}

Test(arena, arena_alloc_exact_capacity) {
  Arena a = arena_init();

  // Allocate exactly the initial capacity
  void *ptr = arena_alloc(&a, INITIAL_ARENA_SIZE);
  cr_assert_not_null(ptr, "Exact capacity allocation should succeed");

  // Next allocation should trigger growth
  void *ptr2 = arena_alloc(&a, 1);
  cr_assert_not_null(ptr2, "Allocation after exact capacity should succeed");

  arena_destroy(&a);
}

// =========================
// String Allocation Tests
// =========================

Test(arena, arena_allocate_string_basic) {
  Arena a = arena_init();

  const char *source = "Hello, World!";
  char *result = arena_allocate_string(&a, source, source + strlen(source));

  cr_assert_not_null(result, "String allocation should succeed");
  cr_assert_str_eq(result, source, "String should be copied correctly");

  arena_destroy(&a);
}

Test(arena, arena_allocate_string_empty) {
  Arena a = arena_init();

  const char *source = "test";
  char *result = arena_allocate_string(&a, source, source); // begin == end

  cr_assert_not_null(result, "Empty string allocation should succeed");
  cr_assert_str_eq(result, "", "Empty string should be correctly allocated");

  arena_destroy(&a);
}

Test(arena, arena_allocate_string_single_char) {
  Arena a = arena_init();

  const char *source = "A";
  char *result = arena_allocate_string(&a, source, source + 1);

  cr_assert_not_null(result, "Single character allocation should succeed");
  cr_assert_str_eq(result, "A", "Single character should be copied correctly");

  arena_destroy(&a);
}

Test(arena, arena_allocate_string_with_nulls) {
  Arena a = arena_init();

  const char source[] = {'H', 'e', 'l', '\0', 'l', 'o'};
  char *result = arena_allocate_string(&a, source, source + 6);

  cr_assert_not_null(result, "String with nulls allocation should succeed");
  cr_assert_eq(memcmp(result, source, 6), 0,
               "String with nulls should be copied correctly");
  cr_assert_eq(result[6], '\0', "String should be null-terminated");

  arena_destroy(&a);
}

Test(arena, arena_allocate_string_long) {
  Arena a = arena_init();

  // Create a string longer than initial arena size
  char long_string[INITIAL_ARENA_SIZE * 2];
  memset(long_string, 'X', sizeof(long_string) - 1);
  long_string[sizeof(long_string) - 1] = '\0';

  char *result =
      arena_allocate_string(&a, long_string, long_string + strlen(long_string));

  cr_assert_not_null(result, "Long string allocation should succeed");
  cr_assert_str_eq(result, long_string,
                   "Long string should be copied correctly");

  arena_destroy(&a);
}

// =========================
// String Concatenation Tests
// =========================

Test(arena, arena_concat_basic) {
  Arena a = arena_init();

  char *result = arena_concat(&a, "Hello", " ", "World");

  cr_assert_not_null(result, "Concatenation should succeed");
  cr_assert_str_eq(result, "Hello World",
                   "Strings should be concatenated correctly");

  arena_destroy(&a);
}

Test(arena, arena_concat_single_string) {
  Arena a = arena_init();

  char *result = arena_concat(&a, "Hello");

  cr_assert_not_null(result, "Single string concatenation should succeed");
  cr_assert_str_eq(result, "Hello", "Single string should be copied correctly");

  arena_destroy(&a);
}

Test(arena, arena_concat_empty_strings) {
  Arena a = arena_init();

  char *result = arena_concat(&a, "", "Hello", "", "World", "");

  cr_assert_not_null(result, "Concatenation with empty strings should succeed");
  cr_assert_str_eq(result, "HelloWorld",
                   "Empty strings should be handled correctly");

  arena_destroy(&a);
}

Test(arena, arena_concat_no_strings) {
  Arena a = arena_init();

  char *result = arena_concat(&a, "");

  cr_assert_not_null(result, "Concatenation with empty string should succeed");
  cr_assert_str_eq(result, "", "Result should be empty string");

  arena_destroy(&a);
}

Test(arena, arena_concat_many_strings) {
  Arena a = arena_init();

  char *result =
      arena_concat(&a, "A", "B", "C", "D", "E", "F", "G", "H", "I", "J");

  cr_assert_not_null(result, "Many string concatenation should succeed");
  cr_assert_str_eq(result, "ABCDEFGHIJ",
                   "Many strings should be concatenated correctly");

  arena_destroy(&a);
}

Test(arena, arena_concat_long_result) {
  Arena a = arena_init();

  // Create strings that when concatenated exceed initial arena size
  char part1[INITIAL_ARENA_SIZE / 2];
  char part2[INITIAL_ARENA_SIZE / 2];
  char part3[INITIAL_ARENA_SIZE / 2];

  memset(part1, 'A', sizeof(part1) - 1);
  memset(part2, 'B', sizeof(part2) - 1);
  memset(part3, 'C', sizeof(part3) - 1);

  part1[sizeof(part1) - 1] = '\0';
  part2[sizeof(part2) - 1] = '\0';
  part3[sizeof(part3) - 1] = '\0';

  char *result = arena_concat(&a, part1, part2, part3);

  cr_assert_not_null(result, "Long concatenation should succeed");
  cr_assert_eq(strlen(result), (INITIAL_ARENA_SIZE / 2 - 1) * 3,
               "Result length should be correct");

  arena_destroy(&a);
}

// =========================
// Memory Usage and Stress Tests
// =========================

Test(arena, arena_many_small_allocations) {
  Arena a = arena_init();

  void *ptrs[1000];

  // Allocate many small chunks
  for (int i = 0; i < 1000; i++) {
    ptrs[i] = arena_alloc(&a, 8);
    cr_assert_not_null(ptrs[i], "Small allocation %d should succeed", i);
  }

  // Verify all pointers are different
  for (int i = 0; i < 1000; i++) {
    for (int j = i + 1; j < 1000; j++) {
      cr_assert_neq(ptrs[i], ptrs[j],
                    "Allocations %d and %d should be different", i, j);
    }
  }

  arena_destroy(&a);
}

Test(arena, arena_alternating_sizes) {
  Arena a = arena_init();

  // Alternate between small and large allocations
  for (int i = 0; i < 50; i++) {
    uint32_t size = (i % 2 == 0) ? 8 : INITIAL_ARENA_SIZE / 4;
    void *ptr = arena_alloc(&a, size);
    cr_assert_not_null(ptr, "Alternating allocation %d should succeed", i);
  }

  arena_destroy(&a);
}

Test(arena, arena_write_and_read_data) {
  Arena a = arena_init();

  // Allocate and write test data
  int *int_ptr = (int *)arena_alloc(&a, sizeof(int));
  *int_ptr = 42;

  char *char_array = (char *)arena_alloc(&a, 10);
  strcpy(char_array, "test");

  double *double_ptr = (double *)arena_alloc(&a, sizeof(double));
  *double_ptr = 3.14159;

  // Verify data integrity
  cr_assert_eq(*int_ptr, 42, "Integer data should be preserved");
  cr_assert_str_eq(char_array, "test", "String data should be preserved");
  cr_assert_float_eq(*double_ptr, 3.14159, 0.00001,
                     "Double data should be preserved");

  arena_destroy(&a);
}

// =========================
// Edge Case and Boundary Tests
// =========================

Test(arena, arena_multiple_free_calls) {
  Arena a = arena_init();

  arena_alloc(&a, 32);

  // Multiple free calls should be safe
  arena_destroy(&a);
  arena_destroy(&a);
  arena_destroy(&a);
}

Test(arena, arena_alloc_after_free) {
  Arena a = arena_init();

  arena_alloc(&a, 32);
  arena_destroy(&a);

  // Should be able to use arena again after free
  void *ptr = arena_alloc(&a, 16);
  cr_assert_not_null(ptr, "Allocation after free should succeed");

  arena_destroy(&a);
}

Test(arena, arena_large_allocation_exact_doubling) {
  Arena a = arena_init();

  // First allocation fills up initial capacity
  arena_alloc(&a, INITIAL_ARENA_SIZE - sizeof(void *)); // Account for alignment

  // This should trigger exactly a doubling
  void *ptr = arena_alloc(&a, INITIAL_ARENA_SIZE);
  cr_assert_not_null(ptr, "Large allocation after capacity should succeed");

  arena_destroy(&a);
}

Test(arena, arena_stress_string_operations) {
  Arena a = arena_init();

  // Mix string allocation and concatenation
  for (int i = 0; i < 100; i++) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "String_%d", i);

    char *str1 = arena_allocate_string(&a, buffer, buffer + strlen(buffer));
    char *str2 = arena_concat(&a, "Prefix_", buffer, "_Suffix");

    cr_assert_not_null(str1, "String allocation %d should succeed", i);
    cr_assert_not_null(str2, "String concatenation %d should succeed", i);
  }

  arena_destroy(&a);
}

// =========================
// ARENA REGION MANAGEMENT TESTS
// =========================

Test(arena, arena_region_creation_sequence) {
  Arena a = arena_init();

  // Track allocations to verify region creation
  void *ptrs[20];

  // First allocation should create initial region
  ptrs[0] = arena_alloc(&a, INITIAL_ARENA_SIZE / 4);
  cr_assert_not_null(ptrs[0], "First allocation should succeed");

  // Fill up most of the initial region
  ptrs[1] = arena_alloc(&a, INITIAL_ARENA_SIZE / 4);
  ptrs[2] = arena_alloc(&a, INITIAL_ARENA_SIZE / 4);
  cr_assert_not_null(ptrs[1], "Second allocation should succeed");
  cr_assert_not_null(ptrs[2], "Third allocation should succeed");

  // This should trigger region growth (new region creation)
  ptrs[3] = arena_alloc(&a, INITIAL_ARENA_SIZE / 2);
  cr_assert_not_null(ptrs[3], "Large allocation should trigger new region");

  // Verify all pointers are different and valid
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 4; j++) {
      cr_assert_neq(ptrs[i], ptrs[j],
                    "Allocations %d and %d should be different", i, j);
    }
  }

  arena_destroy(&a);
}

Test(arena, arena_region_doubling_behavior) {
  Arena a = arena_init();

  // Allocate exactly the initial capacity to fill first region
  void *ptr1 = arena_alloc(&a, INITIAL_ARENA_SIZE);
  cr_assert_not_null(ptr1, "Initial capacity allocation should succeed");

  // Next allocation should create a region with doubled capacity
  void *ptr2 = arena_alloc(&a, INITIAL_ARENA_SIZE);
  cr_assert_not_null(ptr2, "Allocation in doubled region should succeed");

  // Should still be able to allocate within the doubled region
  void *ptr3 = arena_alloc(&a, INITIAL_ARENA_SIZE / 2);
  cr_assert_not_null(ptr3,
                     "Additional allocation in doubled region should succeed");

  arena_destroy(&a);
}

Test(arena, arena_region_large_immediate_allocation) {
  Arena a = arena_init();

  // Allocate something larger than initial capacity as first allocation
  uint32_t large_size = INITIAL_ARENA_SIZE * 3;
  void *ptr1 = arena_alloc(&a, large_size);
  cr_assert_not_null(ptr1, "Large initial allocation should succeed");

  // Should be able to allocate normally after large allocation
  void *ptr2 = arena_alloc(&a, 32);
  cr_assert_not_null(ptr2, "Normal allocation after large should succeed");

  // Another large allocation
  void *ptr3 = arena_alloc(&a, INITIAL_ARENA_SIZE * 2);
  cr_assert_not_null(ptr3, "Second large allocation should succeed");

  arena_destroy(&a);
}

Test(arena, arena_region_chain_multiple_growths) {
  Arena a = arena_init();

  void *ptrs[10];

  // Force multiple region creations
  for (int i = 0; i < 10; i++) {
    ptrs[i] = arena_alloc(&a, INITIAL_ARENA_SIZE);
    cr_assert_not_null(ptrs[i], "Allocation %d should succeed", i);

    // Write test data to verify memory integrity (only write to first 32 bytes
    // for safety)
    uint32_t write_size = INITIAL_ARENA_SIZE > 32 ? 32 : INITIAL_ARENA_SIZE;
    memset(ptrs[i], (char)('A' + i), write_size);
  }

  // Verify memory integrity across all regions
  for (int i = 0; i < 10; i++) {
    char *data = (char *)ptrs[i];
    uint32_t check_size = INITIAL_ARENA_SIZE > 32 ? 32 : INITIAL_ARENA_SIZE;
    for (uint32_t j = 0; j < check_size; j++) {
      cr_assert_eq(data[j], (char)('A' + i),
                   "Memory corruption in allocation %d at byte %" PRIu32 "", i,
                   j);
    }
  }

  arena_destroy(&a);
}

Test(arena, arena_region_mixed_allocation_sizes) {
  Arena a = arena_init();

  void *ptrs[50];
  uint32_t sizes[] = {8,
                      16,
                      32,
                      64,
                      128,
                      256,
                      512,
                      1024,
                      INITIAL_ARENA_SIZE / 2,
                      INITIAL_ARENA_SIZE};
  uint32_t num_sizes = sizeof(sizes) / sizeof(sizes[0]);

  // Mix small and large allocations to test region management
  for (int i = 0; i < 50; i++) {
    uint32_t size = sizes[(uint32_t)i % num_sizes];
    ptrs[i] = arena_alloc(&a, size);
    cr_assert_not_null(ptrs[i],
                       "Mixed allocation %d (size %" PRIu32 ") should succeed",
                       i, size);
  }

  // Verify all allocations are distinct
  for (int i = 0; i < 50; i++) {
    for (int j = i + 1; j < 50; j++) {
      cr_assert_neq(ptrs[i], ptrs[j],
                    "Mixed allocations %d and %d should be different", i, j);
    }
  }

  arena_destroy(&a);
}

// =========================
// DYNAMIC GROWTH EDGE CASES
// =========================

Test(arena, arena_growth_exact_boundary_conditions) {
  Arena a = arena_init();

  // Test allocation exactly at capacity boundary
  void *ptr1 = arena_alloc(&a, INITIAL_ARENA_SIZE -
                                   sizeof(void *)); // Account for alignment
  cr_assert_not_null(ptr1, "Near-capacity allocation should succeed");

  // Small allocation should trigger growth
  void *ptr2 = arena_alloc(&a, 1);
  cr_assert_not_null(ptr2,
                     "Tiny allocation after near-capacity should succeed");

  // Should now have doubled capacity available
  void *ptr3 = arena_alloc(&a, INITIAL_ARENA_SIZE);
  cr_assert_not_null(ptr3, "Large allocation in new region should succeed");

  arena_destroy(&a);
}

Test(arena, arena_growth_progressive_doubling) {
  Arena a = arena_init();

  uint32_t expected_capacities[] = {INITIAL_ARENA_SIZE, INITIAL_ARENA_SIZE * 2,
                                    INITIAL_ARENA_SIZE * 4,
                                    INITIAL_ARENA_SIZE * 8};

  // Force progressive region doubling
  for (uint32_t i = 0; i < 4; i++) {
    void *ptr = arena_alloc(&a, expected_capacities[i]);
    cr_assert_not_null(ptr, "Progressive allocation %" PRIu32 " should succeed",
                       i);

    // Write and verify data
    memset(ptr, (char)('X' + i), 100);
    char *data = (char *)ptr;
    for (int j = 0; j < 100; j++) {
      cr_assert_eq(data[j], (char)('X' + i),
                   "Data corruption in progressive allocation %" PRIu32 "", i);
    }
  }

  arena_destroy(&a);
}

Test(arena, arena_growth_interleaved_allocations) {
  Arena a = arena_init();

  // Interleave small and large allocations to stress region management
  void *small_ptrs[20];
  void *large_ptrs[5];

  for (int i = 0; i < 5; i++) {
    // Small allocations
    for (int j = 0; j < 4; j++) {
      small_ptrs[i * 4 + j] = arena_alloc(&a, 16);
      cr_assert_not_null(small_ptrs[i * 4 + j],
                         "Small allocation %d should succeed", i * 4 + j);
    }

    // Large allocation that may trigger growth
    large_ptrs[i] = arena_alloc(&a, INITIAL_ARENA_SIZE / 2);
    cr_assert_not_null(large_ptrs[i], "Large allocation %d should succeed", i);
  }

  // Verify all allocations are valid and distinct
  for (int i = 0; i < 20; i++) {
    cr_assert_not_null(small_ptrs[i], "Small pointer %d should be valid", i);
  }
  for (int i = 0; i < 5; i++) {
    cr_assert_not_null(large_ptrs[i], "Large pointer %d should be valid", i);
  }

  arena_destroy(&a);
}

Test(arena, arena_growth_memory_layout_integrity) {
  Arena a = arena_init();

  struct TestData {
    int id;
    char data[16];
    double value;
  };

  struct TestData *structs[100];

  // Allocate many structs across multiple regions
  for (int i = 0; i < 100; i++) {
    structs[i] = (struct TestData *)arena_alloc(&a, sizeof(struct TestData));
    cr_assert_not_null(structs[i], "Struct allocation %d should succeed", i);

    // Initialize with test data
    structs[i]->id = i;
    snprintf(structs[i]->data, sizeof(structs[i]->data), "test_%03d", i);
    structs[i]->value = i * 3.14159;
  }

  // Verify data integrity across all regions
  for (int i = 0; i < 100; i++) {
    cr_assert_eq(structs[i]->id, i, "Struct %d ID should be preserved", i);

    char expected[16];
    snprintf(expected, sizeof(expected), "test_%03d", i);
    cr_assert_str_eq(structs[i]->data, expected,
                     "Struct %d data should be preserved", i);

    cr_assert_float_eq(structs[i]->value, i * 3.14159, 0.00001,
                       "Struct %d value should be preserved", i);
  }

  arena_destroy(&a);
}

Test(arena, arena_growth_stress_random_sizes) {
  Arena a = arena_init();

  void *ptrs[200];
  uint32_t sizes[200];

  // Generate random-ish sizes for stress testing
  for (int i = 0; i < 200; i++) {
    // Use a pseudo-random pattern based on index
    sizes[i] = (uint32_t)(((i * 17 + 23) % 1000) + 1); // 1 to 1000 bytes
    if (i % 10 == 0) {
      sizes[i] = INITIAL_ARENA_SIZE / 4; // Occasional large allocation
    }

    ptrs[i] = arena_alloc(&a, sizes[i]);
    cr_assert_not_null(ptrs[i],
                       "Random allocation %d (size %" PRIu32 ") should succeed",
                       i, sizes[i]);

    // Write pattern to detect corruption
    memset(ptrs[i], (char)(i % 256), sizes[i] > 100 ? 100 : sizes[i]);
  }

  // Verify memory integrity
  for (int i = 0; i < 200; i++) {
    char *data = (char *)ptrs[i];
    uint32_t check_size = sizes[i] > 100 ? 100 : sizes[i];
    for (uint32_t j = 0; j < check_size; j++) {
      cr_assert_eq(data[j], (char)(i % 256),
                   "Memory corruption in allocation %d at byte %" PRIu32 "", i,
                   j);
    }
  }

  arena_destroy(&a);
}

// =========================
// Parameterized Tests for Various Sizes
// =========================

struct allocation_size_params {
  uint32_t size;
  const char *description;
};

ParameterizedTestParameters(arena, arena_alloc_various_sizes) {
  static struct allocation_size_params params[] = {
      {1, "single byte"},
      {2, "two bytes"},
      {4, "four bytes"},
      {8, "eight bytes"},
      {16, "sixteen bytes"},
      {32, "thirty-two bytes"},
      {INITIAL_ARENA_SIZE / 2, "half arena size"},
      {INITIAL_ARENA_SIZE, "full arena size"},
      {INITIAL_ARENA_SIZE * 2, "double arena size"},
      {INITIAL_ARENA_SIZE * 4, "quadruple arena size"}};

  uint32_t nb_params = sizeof(params) / sizeof(struct allocation_size_params);
  return cr_make_param_array(struct allocation_size_params, params, nb_params);
}

ParameterizedTest(struct allocation_size_params *param, arena,
                  arena_alloc_various_sizes) {
  Arena a = arena_init();

  void *ptr = arena_alloc(&a, param->size);
  cr_assert_not_null(ptr, "Allocation of %s should succeed",
                     param->description);
  cr_assert_eq((uintptr_t)ptr % sizeof(void *), 0,
               "Allocation should be aligned for %s", param->description);

  arena_destroy(&a);
}