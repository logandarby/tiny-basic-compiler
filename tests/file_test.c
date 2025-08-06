#include "../src/common/file_reader.h"
#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <criterion/parameterized.h>
#include <stdio.h>
#include <string.h>

// Test helper function to collect all words from a FileReader
typedef struct {
  char **lines;
  uint32_t count;
  uint32_t capacity;
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
  for (uint32_t i = 0; i < wl->count; i++) {
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
                               uint32_t expected_count) {
  cr_assert_eq(actual->count, expected_count,
               "Expected %" PRIu32 " lines, got %" PRIu32 "", expected_count,
               actual->count);

  for (uint32_t i = 0; i < expected_count; i++) {
    cr_assert_str_eq(actual->lines[i], expected[i],
                     "Line %" PRIu32 ": expected '%s', got '%s'", i,
                     expected[i], actual->lines[i]);
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

  const char *expected[] = {"first line", "second line", "third line"};
  assert_lines_equal(words, expected, array_size(expected));

  linelist_destroy(words);
  filereader_destroy(&fr);
}

// =================================
// FileReader LINE INDEX Tests
// =================================

Test(file_reader, basic_line_index) {
  FileReader fr =
      filereader_init_from_string("first line\bsecond line\nthird line\n");
  uint32_t line_num = filereader_get_current_line_number(fr);
  cr_assert_eq(
      line_num, NO_LINE_NUMBER,
      "FileReader line number should be initialized with sentinel error value");

  filereader_read_next_line(fr);
  line_num = filereader_get_current_line_number(fr);
  cr_assert_eq(line_num, 1);

  filereader_read_next_line(fr);
  line_num = filereader_get_current_line_number(fr);
  cr_assert_eq(line_num, 2);

  filereader_read_next_line(fr);
  line_num = filereader_get_current_line_number(fr);
  cr_assert_eq(line_num, 3);

  filereader_read_next_line(fr);
  line_num = filereader_get_current_line_number(fr);
  cr_assert_eq(
      line_num, NO_LINE_NUMBER,
      "FileReader line number should be set to Sentinel value after EOF");

  filereader_read_next_line(fr);
  line_num = filereader_get_current_line_number(fr);
  cr_assert_eq(line_num, NO_LINE_NUMBER,
               "FileReader line number should be set to Sentinel value after "
               "multiple reads from EOF");

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
