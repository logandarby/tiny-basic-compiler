#include "../src/common/file_reader.h"
#include <criterion/criterion.h>
#include <criterion/parameterized.h>
#include <stdio.h>
#include <string.h>

// Test helper function to collect all words from a FileReader
typedef struct {
  char **lines;
  size_t count;
  size_t capacity;
} LineList;

static LineList *linelist_create(void) {
  LineList *wl = malloc(sizeof(LineList));
  wl->lines = malloc(sizeof(char *) * 16);
  wl->count = 0;
  wl->capacity = 16;
  return wl;
}

static void linelist_add(LineList *wl, const char *line) {
  if (wl->count >= wl->capacity) {
    wl->capacity *= 2;
    wl->lines = realloc(wl->lines, sizeof(char *) * wl->capacity);
  }
  wl->lines[wl->count] = malloc(strlen(line) + 1);
  strcpy(wl->lines[wl->count], line);
  wl->count++;
}

static void linelist_destroy(LineList *wl) {
  if (!wl)
    return;
  for (size_t i = 0; i < wl->count; i++) {
    free(wl->lines[i]);
  }
  free(wl->lines);
  free(wl);
}

static LineList *read_all_lines(FileReader fr) {
  LineList *lines = linelist_create();
  const char *line;
  while ((line = filereader_read_next_line(fr)) != NULL) {
    linelist_add(lines, line);
  }
  return lines;
}

static void assert_lines_equal(LineList *actual, const char **expected,
                               size_t expected_count) {
  cr_assert_eq(actual->count, expected_count, "Expected %zu lines, got %zu",
               expected_count, actual->count);

  for (size_t i = 0; i < expected_count; i++) {
    cr_assert_str_eq(actual->lines[i], expected[i],
                     "Line %zu: expected '%s', got '%s'", i, expected[i],
                     actual->lines[i]);
  }
}

// =========================
// BASIC FUNCTIONALITY TESTS
// =========================

Test(file_reader, basic_single_word) {
  FileReader fr = filereader_init_from_string("hello");
  LineList *words = read_all_lines(fr);

  const char *expected[] = {"hello"};
  assert_lines_equal(words, expected, 1);

  linelist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, basic_multiple_words) {
  FileReader fr = filereader_init_from_string("hello world test");
  LineList *words = read_all_lines(fr);

  const char *expected[] = {"hello world test"};
  assert_lines_equal(words, expected, array_size(expected));

  linelist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, basic_multiple_lines) {
  FileReader fr =
      filereader_init_from_string("first line\nsecond line\nthird line");
  LineList *words = read_all_lines(fr);

  const char *expected[] = {"first line\n", "second line\n", "third line"};
  assert_lines_equal(words, expected, array_size(expected));

  linelist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// ERROR INJECTION TESTS
// =========================

Test(file_reader, double_destruction_safety) {
  // Test that double-destruction doesn't crash
  FileReader fr = filereader_init_from_string("test");
  filereader_destroy(&fr);
  filereader_destroy(&fr);  // Should not crash (fr is now NULL)
  filereader_destroy(NULL); // Should not crash
}

Test(file_reader, null_pointer_chaos) {
  // Test all functions with NULL pointers
  cr_assert_null(filereader_read_next_line(NULL));
  cr_assert_eq(filereader_get_error(NULL), FR_ERR_FILE_NOT_FOUND);
  filereader_destroy(NULL); // Should not crash
}