#include "../src/frontend/lexer/token.h"
#include <criterion/criterion.h>
#include <criterion/parameterized.h>

// =========================
// INITIALIZATION AND BASIC OPERATIONS
// =========================

// Mock FileLocation for testing
static const FileLocation fl = {
    .line = 0,
    .col = 0,
};

Test(token_array, initialization) {
  TokenArray ta = token_array_init();

  cr_assert_not_null(ta, "TokenArray should initialize successfully");
  cr_assert_eq(token_array_length(ta), 0,
               "New TokenArray should have length 0");
  cr_assert_gt(token_array_capacity(ta), 0,
               "New TokenArray should have positive capacity");
  cr_assert(token_array_is_empty(ta), "New TokenArray should be empty");

  token_array_destroy(&ta);
}

Test(token_array, initial_capacity) {
  TokenArray ta = token_array_init();

  // Should start with INIT_CAPACITY (512)
  cr_assert_eq(token_array_capacity(ta), 512, "Initial capacity should be 512");

  token_array_destroy(&ta);
}

Test(token_array, single_push) {
  TokenArray ta = token_array_init();

  token_array_push_simple(ta, TOKEN_PLUS, fl);

  cr_assert_eq(token_array_length(ta), 1, "Length should be 1 after one push");
  cr_assert_eq(token_array_capacity(ta), 512, "Capacity should remain 512");
  cr_assert(!token_array_is_empty(ta), "Array should not be empty after push");
  cr_assert_eq(token_array_at(ta, 0).type, TOKEN_PLUS,
               "First element should be TOKEN_PLUS");

  token_array_destroy(&ta);
}

Test(token_array, multiple_pushes_within_capacity) {
  TokenArray ta = token_array_init();

  // Add elements without exceeding initial capacity
  for (int i = 0; i < 100; i++) {
    token_array_push_simple(ta, TOKEN_PLUS, fl);
  }

  cr_assert_eq(token_array_length(ta), 100, "Length should be 100");
  cr_assert_eq(token_array_capacity(ta), 512, "Capacity should still be 512");

  // Verify all elements
  for (size_t i = 0; i < 100; i++) {
    cr_assert_eq(token_array_at(ta, i).type, TOKEN_PLUS,
                 "Element %ld should be TOKEN_PLUS", i);
  }

  token_array_destroy(&ta);
}

// =========================
// DYNAMIC RESIZING TESTS
// =========================

Test(token_array, resize_at_capacity_boundary) {
  TokenArray ta = token_array_init();

  // Fill exactly to initial capacity
  for (size_t i = 0; i < 512; i++) {
    token_array_push_simple(ta, TOKEN_MINUS, fl);
  }

  cr_assert_eq(token_array_length(ta), 512, "Length should be 512");
  cr_assert_eq(token_array_capacity(ta), 512, "Capacity should still be 512");

  // This push should trigger resize
  token_array_push_simple(ta, TOKEN_MULT, fl);

  cr_assert_eq(token_array_length(ta), 513,
               "Length should be 513 after resize");
  cr_assert_eq(token_array_capacity(ta), 1024,
               "Capacity should double to 1024");

  // Verify all elements are intact
  for (size_t i = 0; i < 512; i++) {
    cr_assert_eq(token_array_at(ta, i).type, TOKEN_MINUS,
                 "Element %zu should be TOKEN_MINUS", i);
  }
  cr_assert_eq(token_array_at(ta, 512).type, TOKEN_MULT,
               "Last element should be TOKEN_MULT");

  token_array_destroy(&ta);
}

Test(token_array, multiple_resizes) {
  TokenArray ta = token_array_init();

  // Test multiple resize operations
  const size_t target_size = 2048 + 10; // Force multiple resizes

  for (size_t i = 0; i < target_size; i++) {
    enum TOKEN token =
        (enum TOKEN)(TOKEN_PLUS + (i % 4)); // Cycle through some tokens
    token_array_push_simple(ta, token, fl);
  }

  cr_assert_eq(token_array_length(ta), target_size,
               "Length should match target size");
  cr_assert(token_array_capacity(ta) >= target_size,
            "Capacity should be at least target size");
  cr_assert_eq(token_array_capacity(ta), 4096,
               "Capacity should be 4096 after multiple resizes");

  // Verify elements are intact after multiple resizes
  for (size_t i = 0; i < target_size; i++) {
    enum TOKEN expected = (enum TOKEN)(TOKEN_PLUS + (i % 4));
    cr_assert_eq(token_array_at(ta, i).type, expected,
                 "Element %zu should be correct after resizes", i);
  }

  token_array_destroy(&ta);
}

Test(token_array, resize_sequence_validation) {
  TokenArray ta = token_array_init();

  // Track capacity changes through multiple resizes
  size_t expected_capacities[] = {512, 1024, 2048, 4096, 8192};
  size_t capacity_index = 0;

  cr_assert_eq(token_array_capacity(ta), expected_capacities[capacity_index],
               "Initial capacity should be %zu",
               expected_capacities[capacity_index]);

  for (size_t i = 1; i <= 4096; i++) {
    token_array_push_simple(ta, TOKEN_DIV, fl);

    // Check if we've hit a resize boundary
    if (i == expected_capacities[capacity_index]) {
      capacity_index++;
      if (capacity_index <
          sizeof(expected_capacities) / sizeof(expected_capacities[0])) {
        // Next push should trigger resize
        token_array_push_simple(ta, TOKEN_GT, fl);
        i++;
        cr_assert_eq(token_array_capacity(ta),
                     expected_capacities[capacity_index],
                     "Capacity should be %zu after resize",
                     expected_capacities[capacity_index]);
      }
    }
  }

  token_array_destroy(&ta);
}

// =========================
// STRESS TESTS
// =========================

Test(token_array, large_scale_operations) {
  TokenArray ta = token_array_init();

  const size_t large_size = 10000;

  // Add many elements
  for (size_t i = 0; i < large_size; i++) {
    token_array_push_simple(ta, (enum TOKEN)(TOKEN_PLUS + (i % 14)),
                            fl); // Cycle through operator tokens
  }

  cr_assert_eq(token_array_length(ta), large_size, "Length should be %zu",
               large_size);
  cr_assert(token_array_capacity(ta) >= large_size,
            "Capacity should accommodate all elements");

  // Verify all elements
  for (size_t i = 0; i < large_size; i++) {
    enum TOKEN expected = (enum TOKEN)(TOKEN_PLUS + (i % 14));
    cr_assert_eq(token_array_at(ta, i).type, expected,
                 "Element %zu should be correct", i);
  }

  token_array_destroy(&ta);
}

Test(token_array, alternating_token_stress) {
  TokenArray ta = token_array_init();

  // Alternate between different token types to stress the storage
  const size_t iterations = 1000;

  for (size_t i = 0; i < iterations; i++) {
    token_array_push_simple(ta, TOKEN_PLUS, fl);
    token_array_push_simple(ta, TOKEN_UNKNOWN, fl);
    token_array_push_simple(ta, TOKEN_NUMBER, fl);
    token_array_push_simple(ta, TOKEN_IDENT, fl);
    token_array_push_simple(ta, TOKEN_IF, fl);
  }

  cr_assert_eq(token_array_length(ta), iterations * 5, "Length should be %zu",
               iterations * 5);

  // Verify the alternating pattern
  for (size_t i = 0; i < iterations; i++) {
    cr_assert_eq(token_array_at(ta, i * 5 + 0).type, TOKEN_PLUS,
                 "Pattern verification failed at %zu", i);
    cr_assert_eq(token_array_at(ta, i * 5 + 1).type, TOKEN_UNKNOWN,
                 "Pattern verification failed at %zu", i);
    cr_assert_eq(token_array_at(ta, i * 5 + 2).type, TOKEN_NUMBER,
                 "Pattern verification failed at %zu", i);
    cr_assert_eq(token_array_at(ta, i * 5 + 3).type, TOKEN_IDENT,
                 "Pattern verification failed at %zu", i);
    cr_assert_eq(token_array_at(ta, i * 5 + 4).type, TOKEN_IF,
                 "Pattern verification failed at %zu", i);
  }

  token_array_destroy(&ta);
}

// =========================
// EDGE CASES
// =========================

Test(token_array, immediate_resize_trigger) {
  TokenArray ta = token_array_init();

  // Fill to exactly initial capacity
  for (size_t i = 0; i < 512; i++) {
    token_array_push_simple(ta, TOKEN_LET, fl);
  }

  size_t capacity_before = token_array_capacity(ta);
  cr_assert_eq(capacity_before, 512, "Should be at capacity limit");

  // One more push should double capacity
  token_array_push_simple(ta, TOKEN_GOTO, fl);

  size_t capacity_after = token_array_capacity(ta);
  cr_assert_eq(capacity_after, 1024, "Capacity should double");
  cr_assert_eq(token_array_length(ta), 513, "Length should be 513");

  token_array_destroy(&ta);
}

Test(token_array, single_element_to_resize) {
  TokenArray ta = token_array_init();

  // Add exactly enough elements to trigger one resize
  for (size_t i = 0; i <= 512; i++) { // 513 elements total
    token_array_push_simple(ta, TOKEN_PRINT, fl);
  }

  cr_assert_eq(token_array_length(ta), 513, "Should have 513 elements");
  cr_assert_eq(token_array_capacity(ta), 1024, "Should have resized to 1024");

  token_array_destroy(&ta);
}

Test(token_array, exact_power_of_two_boundaries) {
  TokenArray ta = token_array_init();

  // Test at exact power-of-2 boundaries
  size_t boundaries[] = {512, 1024, 2048};

  for (size_t b = 0; b < sizeof(boundaries) / sizeof(boundaries[0]); b++) {
    // Clear and test each boundary
    token_array_destroy(&ta);
    ta = token_array_init();

    size_t target = boundaries[b];

    // Fill to exact boundary
    for (size_t i = 0; i < target; i++) {
      token_array_push_simple(ta, TOKEN_WHILE, fl);
    }

    cr_assert_eq(token_array_length(ta), target,
                 "Should have exactly %zu elements", target);
    cr_assert_eq(token_array_capacity(ta), target,
                 "Capacity should be exactly %zu", target);

    // One more should trigger resize
    token_array_push_simple(ta, TOKEN_ENDWHILE, fl);
    cr_assert_eq(token_array_capacity(ta), target * 2, "Should double to %zu",
                 target * 2);
  }

  token_array_destroy(&ta);
}

// =========================
// MEMORY AND ERROR HANDLING
// =========================

Test(token_array, destroy_and_null_check) {
  TokenArray ta = token_array_init();

  token_array_push_simple(ta, TOKEN_REPEAT, fl);
  cr_assert_not_null(ta, "TokenArray should not be null before destroy");

  token_array_destroy(&ta);
  cr_assert_null(ta, "TokenArray should be null after destroy");

  // Double destroy should be safe
  token_array_destroy(&ta);
  cr_assert_null(ta, "TokenArray should remain null after double destroy");
}

Test(token_array, null_pointer_safety) {
  // All functions should handle NULL gracefully or have defined behavior
  TokenArray null_ta = NULL;

  // These should not crash
  token_array_destroy(&null_ta);
  token_array_destroy(NULL);

  // Note: Other functions with NULL input have undefined behavior in this
  // implementation but they shouldn't crash in practice
}

Test(token_array, repeated_create_destroy) {
  // Test for memory leaks with repeated creation/destruction
  for (int i = 0; i < 100; i++) {
    TokenArray ta = token_array_init();

    // Add some elements
    for (int j = 0; j < 10; j++) {
      token_array_push_simple(ta, TOKEN_INPUT, fl);
    }

    cr_assert_eq(token_array_length(ta), 10, "Should have 10 elements");

    token_array_destroy(&ta);
    cr_assert_null(ta, "Should be null after destroy");
  }
}

// =========================
// DATA INTEGRITY TESTS
// =========================

Test(token_array, data_integrity_across_resizes) {
  TokenArray ta = token_array_init();

  // Create a known pattern that will survive multiple resizes
  const size_t pattern_size = 1000;
  enum TOKEN pattern[pattern_size];

  // Fill pattern with predictable data
  for (size_t i = 0; i < pattern_size; i++) {
    pattern[i] =
        (enum TOKEN)(TOKEN_PLUS + (i % 20)); // Use first 20 token types
    token_array_push_simple(ta, pattern[i], fl);
  }

  // Add more elements to force multiple resizes
  for (size_t i = 0; i < 2000; i++) {
    token_array_push_simple(ta, TOKEN_UNKNOWN, fl);
  }

  // Verify original pattern is intact
  for (size_t i = 0; i < pattern_size; i++) {
    cr_assert_eq(token_array_at(ta, i).type, pattern[i],
                 "Pattern element %zu should be preserved across resizes", i);
  }

  // Verify the added elements
  for (size_t i = pattern_size; i < pattern_size + 2000; i++) {
    cr_assert_eq(token_array_at(ta, i).type, TOKEN_UNKNOWN,
                 "Added element %zu should be TOKEN_UNKNOWN", i);
  }

  token_array_destroy(&ta);
}

Test(token_array, capacity_never_shrinks) {
  TokenArray ta = token_array_init();

  size_t initial_capacity = token_array_capacity(ta);

  // Force a resize
  for (size_t i = 0; i <= initial_capacity; i++) {
    token_array_push_simple(ta, TOKEN_ELSE, fl);
  }

  size_t after_resize = token_array_capacity(ta);
  cr_assert_gt(after_resize, initial_capacity,
               "Capacity should increase after resize");

  // Note: This implementation doesn't shrink capacity, so we can't test
  // shrinking But we can verify capacity never decreases inadvertently

  token_array_destroy(&ta);
}

// =========================
// PERFORMANCE CHARACTERISTIC TESTS
// =========================

Test(token_array, amortized_growth_pattern) {
  TokenArray ta = token_array_init();

  size_t previous_capacity = token_array_capacity(ta);
  size_t resize_count = 0;

  // Add elements and track capacity changes
  for (size_t i = 0; i < 5000; i++) {
    token_array_push_simple(ta, TOKEN_THEN, fl);

    size_t current_capacity = token_array_capacity(ta);
    if (current_capacity > previous_capacity) {
      resize_count++;
      // Verify doubling behavior
      cr_assert_eq(current_capacity, previous_capacity * 2,
                   "Capacity should double on resize");
      previous_capacity = current_capacity;
    }
  }

  // Should have a reasonable number of resizes (log2 growth)
  cr_assert_lt(resize_count, 10, "Should not have excessive resizes");
  cr_assert_gt(resize_count, 0, "Should have at least one resize");

  token_array_destroy(&ta);
}
