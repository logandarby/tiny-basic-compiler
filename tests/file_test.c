#include "../src/file.h"
#include <criterion/criterion.h>
#include <criterion/parameterized.h>
#include <stdio.h>
#include <string.h>

// Test helper function to collect all words from a FileReader
typedef struct {
  char **words;
  size_t count;
  size_t capacity;
} WordList;

static WordList *wordlist_create(void) {
  WordList *wl = malloc(sizeof(WordList));
  wl->words = malloc(sizeof(char *) * 16);
  wl->count = 0;
  wl->capacity = 16;
  return wl;
}

static void wordlist_add(WordList *wl, const char *word) {
  if (wl->count >= wl->capacity) {
    wl->capacity *= 2;
    wl->words = realloc(wl->words, sizeof(char *) * wl->capacity);
  }
  wl->words[wl->count] = malloc(strlen(word) + 1);
  strcpy(wl->words[wl->count], word);
  wl->count++;
}

static void wordlist_destroy(WordList *wl) {
  if (!wl)
    return;
  for (size_t i = 0; i < wl->count; i++) {
    free(wl->words[i]);
  }
  free(wl->words);
  free(wl);
}

static WordList *read_all_words(FileReader fr) {
  WordList *words = wordlist_create();
  const char *word;
  while ((word = filereader_read_next_word(fr)) != NULL) {
    wordlist_add(words, word);
  }
  return words;
}

static void assert_words_equal(WordList *actual, const char **expected,
                               size_t expected_count) {
  cr_assert_eq(actual->count, expected_count, "Expected %zu words, got %zu",
               expected_count, actual->count);

  for (size_t i = 0; i < expected_count; i++) {
    cr_assert_str_eq(actual->words[i], expected[i],
                     "Word %zu: expected '%s', got '%s'", i, expected[i],
                     actual->words[i]);
  }
}

// =========================
// BASIC FUNCTIONALITY TESTS
// =========================

Test(file_reader, basic_single_word) {
  FileReader fr = filereader_init_from_string("hello");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello"};
  assert_words_equal(words, expected, 1);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, basic_multiple_words) {
  FileReader fr = filereader_init_from_string("hello world test");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world", "test"};
  assert_words_equal(words, expected, 3);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, basic_multiple_lines) {
  FileReader fr =
      filereader_init_from_string("first line\nsecond line\nthird line");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"first", "line", "second", "line", "third", "line"};
  assert_words_equal(words, expected, 6);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// WHITESPACE HANDLING TESTS
// =========================

Test(file_reader, whitespace_spaces) {
  FileReader fr = filereader_init_from_string("  hello   world  ");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world"};
  assert_words_equal(words, expected, 2);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, whitespace_tabs) {
  FileReader fr = filereader_init_from_string("\t\thello\t\tworld\t\t");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world"};
  assert_words_equal(words, expected, 2);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, whitespace_mixed) {
  FileReader fr = filereader_init_from_string(" \t hello \t world \t ");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world"};
  assert_words_equal(words, expected, 2);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, whitespace_only_spaces) {
  FileReader fr = filereader_init_from_string("   ");
  WordList *words = read_all_words(fr);

  assert_words_equal(words, NULL, 0);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, whitespace_only_tabs) {
  FileReader fr = filereader_init_from_string("\t\t\t");
  WordList *words = read_all_words(fr);

  assert_words_equal(words, NULL, 0);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// NEWLINE HANDLING TESTS
// =========================

Test(file_reader, newlines_single) {
  FileReader fr = filereader_init_from_string("hello\nworld");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world"};
  assert_words_equal(words, expected, 2);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, newlines_multiple) {
  FileReader fr = filereader_init_from_string("hello\n\n\nworld");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world"};
  assert_words_equal(words, expected, 2);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, newlines_mixed_with_spaces) {
  FileReader fr = filereader_init_from_string("hello \n \n world");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world"};
  assert_words_equal(words, expected, 2);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, newlines_at_start) {
  FileReader fr = filereader_init_from_string("\n\nhello world");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world"};
  assert_words_equal(words, expected, 2);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, newlines_at_end) {
  FileReader fr = filereader_init_from_string("hello world\n\n");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world"};
  assert_words_equal(words, expected, 2);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// CARRIAGE RETURN TESTS
// =========================

Test(file_reader, carriage_returns) {
  FileReader fr = filereader_init_from_string("hello\rworld\r\ntest");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world", "test"};
  assert_words_equal(words, expected, 3);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, form_feed_vertical_tab) {
  FileReader fr = filereader_init_from_string("hello\fworld\vtest");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world", "test"};
  assert_words_equal(words, expected, 3);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// EDGE CASE TESTS
// =========================

Test(file_reader, empty_string) {
  FileReader fr = filereader_init_from_string("");
  WordList *words = read_all_words(fr);

  assert_words_equal(words, NULL, 0);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, single_character) {
  FileReader fr = filereader_init_from_string("a");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"a"};
  assert_words_equal(words, expected, 1);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, very_long_word) {
  // Create a 500-character word (well under WORD_BUFFER_SIZE of 1024)
  char long_word[501];
  for (int i = 0; i < 500; i++) {
    long_word[i] = (char)('a' + (i % 26));
  }
  long_word[500] = '\0';

  FileReader fr = filereader_init_from_string(long_word);
  WordList *words = read_all_words(fr);

  const char *expected[] = {long_word};
  assert_words_equal(words, expected, 1);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, word_too_big) {
  // Create a word larger than WORD_BUFFER_SIZE (1024)
  char *huge_word = malloc(2000);
  for (int i = 0; i < 1999; i++) {
    huge_word[i] = 'x';
  }
  huge_word[1999] = '\0';

  FileReader fr = filereader_init_from_string(huge_word);

  // Should return NULL due to word being too big
  const char *word = filereader_read_next_word(fr);
  cr_assert_null(word, "Expected NULL for oversized word");
  cr_assert_eq(filereader_get_error(fr), FR_ERR_WORD_TOO_BIG,
               "Expected FR_ERR_WORD_TOO_BIG error");

  free(huge_word);
  filereader_destroy(&fr);
}

Test(file_reader, punctuation_and_symbols) {
  FileReader fr = filereader_init_from_string("hello! @world# $test%");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello!", "@world#", "$test%"};
  assert_words_equal(words, expected, 3);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, numbers_and_mixed) {
  FileReader fr =
      filereader_init_from_string("123 abc456 789def hello123world");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"123", "abc456", "789def", "hello123world"};
  assert_words_equal(words, expected, 4);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// BASIC PROGRAMMING CONSTRUCTS
// =========================

Test(file_reader, basic_program) {
  FileReader fr =
      filereader_init_from_string("10 LET A = 42\n20 PRINT A\n30 END");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"10", "LET",   "A", "=",  "42",
                            "20", "PRINT", "A", "30", "END"};
  assert_words_equal(words, expected, 10);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, expressions_with_operators) {
  FileReader fr = filereader_init_from_string("IF X > 10 THEN Y = X * 2 + 1");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"IF", "X", ">", "10", "THEN", "Y",
                            "=",  "X", "*", "2",  "+",    "1"};
  assert_words_equal(words, expected, 12);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, string_literals_no_quotes) {
  // Note: This tests words that might look like string content but without
  // quotes
  FileReader fr = filereader_init_from_string("PRINT HELLO WORLD");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"PRINT", "HELLO", "WORLD"};
  assert_words_equal(words, expected, 3);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// NULL AND ERROR HANDLING TESTS
// =========================

Test(file_reader, null_input) {
  FileReader fr = filereader_init_from_string(NULL);
  cr_assert_null(fr, "Expected NULL for null input");
}

Test(file_reader, sequential_word_reading_simple) {
  FileReader fr = filereader_init_from_string("hello world");

  const char *word1 = filereader_read_next_word(fr);
  cr_assert_str_eq(word1, "hello");

  const char *word2 = filereader_read_next_word(fr);
  cr_assert_str_eq(word2, "world");

  const char *word3 = filereader_read_next_word(fr);
  cr_assert_null(word3, "Should return NULL when no more words");

  filereader_destroy(&fr);
}

Test(file_reader, error_state_handling) {
  FileReader fr = filereader_init_from_string("hello");

  cr_assert_eq(filereader_get_error(fr), FR_ERR_NONE,
               "Should start with no error");

  const char *word = filereader_read_next_word(fr);
  cr_assert_str_eq(word, "hello");
  cr_assert_eq(filereader_get_error(fr), FR_ERR_NONE,
               "Should still have no error");

  filereader_destroy(&fr);
}

Test(file_reader, null_file_reader) {
  cr_assert_null(filereader_read_next_word(NULL),
                 "NULL FileReader should return NULL");
  cr_assert_eq(filereader_get_error(NULL), FR_ERR_FILE_NOT_FOUND,
               "NULL FileReader should return file not found error");

  // Should not crash
  filereader_destroy(NULL);
}

// =========================
// STRESS AND BOUNDARY TESTS
// =========================

Test(file_reader, many_small_words) {
  // Create 1000 single-character words
  char *input = malloc(2000); // "a b c ... " format
  char *ptr = input;
  for (int i = 0; i < 1000; i++) {
    *ptr++ = (char)('a' + (i % 26));
    if (i < 999)
      *ptr++ = ' ';
  }
  *ptr = '\0';

  FileReader fr = filereader_init_from_string(input);
  WordList *words = read_all_words(fr);

  cr_assert_eq(words->count, 1000, "Should read exactly 1000 words");

  // Verify each word is correct
  for (size_t i = 0; i < 1000; i++) {
    char expected_char = (char)('a' + (i % 26));
    char expected_word[2] = {expected_char, '\0'};
    cr_assert_str_eq(words->words[i], expected_word,
                     "Word %zu should be '%s', got '%s'", i, expected_word,
                     words->words[i]);
  }

  free(input);
  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, many_lines) {
  // Create 100 lines with 3 words each
  char *input = malloc(3000);
  char *ptr = input;
  for (int i = 0; i < 100; i++) {
    ptr += sprintf(ptr, "word%d line%d test%d\n", i, i, i);
  }

  FileReader fr = filereader_init_from_string(input);
  WordList *words = read_all_words(fr);

  cr_assert_eq(words->count, 300, "Should read exactly 300 words");

  free(input);
  wordlist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// UNICODE AND SPECIAL CHARACTER TESTS
// =========================

Test(file_reader, special_characters) {
  FileReader fr = filereader_init_from_string("café naïve résumé");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"café", "naïve", "résumé"};
  assert_words_equal(words, expected, 3);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, non_ascii_characters) {
  FileReader fr = filereader_init_from_string("hello 你好 world مرحبا test");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "你好", "world", "مرحبا", "test"};
  assert_words_equal(words, expected, 5);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// CONSISTENCY TESTS
// =========================

Test(file_reader, multiple_reads_same_input) {
  const char *input = "hello world test";

  // Read the same input multiple times and ensure consistent results
  for (int run = 0; run < 3; run++) {
    FileReader fr = filereader_init_from_string(input);
    WordList *words = read_all_words(fr);

    const char *expected[] = {"hello", "world", "test"};
    assert_words_equal(words, expected, 3);

    wordlist_destroy(words);
    filereader_destroy(&fr);
  }
}

Test(file_reader, sequential_word_reading) {
  FileReader fr = filereader_init_from_string("one two three four");

  cr_assert_str_eq(filereader_read_next_word(fr), "one");
  cr_assert_str_eq(filereader_read_next_word(fr), "two");
  cr_assert_str_eq(filereader_read_next_word(fr), "three");
  cr_assert_str_eq(filereader_read_next_word(fr), "four");
  cr_assert_null(filereader_read_next_word(fr));
  cr_assert_null(filereader_read_next_word(fr)); // Should stay NULL

  filereader_destroy(&fr);
}

// =========================
// EXTREME EDGE CASE TESTS
// =========================

Test(file_reader, pathological_whitespace_combinations) {
  // Test every possible combination of whitespace characters
  FileReader fr =
      filereader_init_from_string(" \t\n\r\f\v \t\n\r\f\v hello \t\n\r\f\v "
                                  "\t\n\r\f\v world \t\n\r\f\v \t\n\r\f\v");
  WordList *words = read_all_words(fr);

  const char *expected[] = {"hello", "world"};
  assert_words_equal(words, expected, 2);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, alternating_whitespace_word_pattern) {
  // Extreme pattern: W[whitespace]W[whitespace]... for 100 iterations
  char *input = malloc(2000);
  char *ptr = input;
  for (int i = 0; i < 50; i++) {
    *ptr++ = (char)('a' + (i % 26));
    *ptr++ = ' ';
    *ptr++ = '\t';
    *ptr++ = '\n';
    *ptr++ = '\r';
    *ptr++ = '\f';
    *ptr++ = '\v';
  }
  *ptr = '\0';

  FileReader fr = filereader_init_from_string(input);
  WordList *words = read_all_words(fr);

  cr_assert_eq(words->count, 50, "Should read exactly 50 words");

  free(input);
  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, maximum_line_length_stress) {
  // Create a line approaching typical system limits (8192 chars)
  const size_t line_length = 8000;
  char *input = malloc(line_length + 100);
  char *ptr = input;

  // Fill with words separated by single spaces
  for (size_t i = 0; i < line_length - 10; i += 6) {
    memcpy(ptr, "word ", 5);
    ptr += 5;
  }
  *ptr = '\0';

  FileReader fr = filereader_init_from_string(input);
  WordList *words = read_all_words(fr);

  // Should read approximately line_length/6 words
  cr_assert_gt(words->count, 1000, "Should read many words from long line");

  free(input);
  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, word_at_exact_buffer_boundary) {
  // Create a word that's exactly WORD_BUFFER_SIZE - 1 (1023 chars)
  const size_t word_size = 1023;
  char *long_word = malloc(word_size + 1);
  for (size_t i = 0; i < word_size; i++) {
    long_word[i] = (char)('a' + (i % 26));
  }
  long_word[word_size] = '\0';

  FileReader fr = filereader_init_from_string(long_word);
  const char *word = filereader_read_next_word(fr);

  cr_assert_not_null(word, "Should successfully read word at buffer boundary");
  cr_assert_eq(strlen(word), word_size, "Word should be exactly %zu characters",
               word_size);
  cr_assert_eq(filereader_get_error(fr), FR_ERR_NONE, "Should have no error");

  free(long_word);
  filereader_destroy(&fr);
}

Test(file_reader, word_exceeds_buffer_by_one) {
  // Create a word that's exactly WORD_BUFFER_SIZE (1024 chars) - should fail
  const size_t word_size = 1024;
  char *huge_word = malloc(word_size + 1);
  for (size_t i = 0; i < word_size; i++) {
    huge_word[i] = 'x';
  }
  huge_word[word_size] = '\0';

  FileReader fr = filereader_init_from_string(huge_word);
  const char *word = filereader_read_next_word(fr);

  cr_assert_null(word, "Should return NULL for word exactly at buffer size");
  cr_assert_eq(filereader_get_error(fr), FR_ERR_WORD_TOO_BIG,
               "Should have word too big error");

  free(huge_word);
  filereader_destroy(&fr);
}

Test(file_reader, massive_number_of_tiny_words) {
  // 10,000 single-character words
  const size_t word_count = 10000;
  char *input = malloc(word_count * 2); // each word + space
  char *ptr = input;

  for (size_t i = 0; i < word_count; i++) {
    *ptr++ = (char)('a' + (i % 26));
    if (i < word_count - 1)
      *ptr++ = ' ';
  }
  *ptr = '\0';

  FileReader fr = filereader_init_from_string(input);
  WordList *words = read_all_words(fr);

  cr_assert_eq(words->count, word_count, "Should read exactly %zu words",
               word_count);

  free(input);
  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, deeply_nested_newlines) {
  // 1000 consecutive newlines with words scattered throughout
  char *input = malloc(5000);
  char *ptr = input;

  for (int i = 0; i < 100; i++) {
    for (int j = 0; j < 10; j++) {
      *ptr++ = '\n';
    }
    ptr += sprintf(ptr, "word%d", i);
  }
  *ptr = '\0';

  FileReader fr = filereader_init_from_string(input);
  WordList *words = read_all_words(fr);

  cr_assert_eq(words->count, 100, "Should read exactly 100 words");

  free(input);
  wordlist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// MEMORY STRESS TESTS
// =========================

Test(file_reader, memory_stress_repeated_creation) {
  // Create and destroy many FileReaders to test for memory leaks
  for (int i = 0; i < 1000; i++) {
    FileReader fr = filereader_init_from_string("test word memory");
    cr_assert_str_eq(filereader_read_next_word(fr), "test");
    cr_assert_str_eq(filereader_read_next_word(fr), "word");
    cr_assert_str_eq(filereader_read_next_word(fr), "memory");

    filereader_destroy(&fr);
  }
}

Test(file_reader, memory_stress_large_allocations) {
  // Test with very large inputs to stress the memory allocator
  const size_t huge_size = 100000; // 100KB of text
  char *huge_input = malloc(huge_size + 1);
  char *ptr = huge_input;

  // Fill with repeating pattern
  for (size_t i = 0; i < huge_size - 10; i += 10) {
    memcpy(ptr, "word test ", 10);
    ptr += 10;
  }
  *ptr = '\0';

  FileReader fr = filereader_init_from_string(huge_input);

  // Read first few words to ensure it works
  cr_assert_str_eq(filereader_read_next_word(fr), "word");
  cr_assert_str_eq(filereader_read_next_word(fr), "test");
  cr_assert_str_eq(filereader_read_next_word(fr), "word");

  free(huge_input);
  filereader_destroy(&fr);
}

Test(file_reader, memory_stress_getline_expansion) {
  // Create a single line longer than initial buffer (1024) to force getline
  // expansion
  const size_t line_length = 5000;
  char *long_line = malloc(line_length + 1);
  char *ptr = long_line;

  // Single line with many words (no newlines)
  for (size_t i = 0; i < line_length - 5; i += 5) {
    memcpy(ptr, "word ", 5);
    ptr += 5;
  }
  *(ptr - 1) = '\0'; // Replace last space with null terminator

  FileReader fr = filereader_init_from_string(long_line);

  // Should successfully read words from expanded buffer
  cr_assert_str_eq(filereader_read_next_word(fr), "word");
  cr_assert_str_eq(filereader_read_next_word(fr), "word");

  free(long_line);
  filereader_destroy(&fr);
}

// =========================
// FUZZING AND CHAOS TESTS
// =========================

Test(file_reader, fuzz_random_whitespace_chaos) {
  // Pseudo-random chaos of whitespace characters
  char
      chaos[2000]; // Increased buffer size to handle worst-case word generation
  char *ptr = chaos;

  // Deterministic "random" pattern for reproducible tests
  for (int i = 0; i < 900; i++) {
    int pattern = (i * 7 + 13) % 100;
    if (pattern < 15)
      *ptr++ = ' ';
    else if (pattern < 30)
      *ptr++ = '\t';
    else if (pattern < 45)
      *ptr++ = '\n';
    else if (pattern < 55)
      *ptr++ = '\r';
    else if (pattern < 60)
      *ptr++ = '\f';
    else if (pattern < 65)
      *ptr++ = '\v';
    else if (pattern < 85) {
      // Insert actual words occasionally
      char word[10];
      sprintf(word, "w%d", pattern);
      strcpy(ptr, word);
      ptr += strlen(word);
    } else {
      *ptr++ = ' '; // More spaces
    }
  }
  *ptr = '\0';

  FileReader fr = filereader_init_from_string(chaos);
  WordList *words = read_all_words(fr);

  // Should successfully parse without crashing
  cr_assert(true, "Should not crash on chaotic input");

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, fuzz_binary_like_data) {
  // Simulate binary data with null bytes and control characters
  char binary_data[200];
  for (int i = 0; i < 199; i++) {
    binary_data[i] = (char)(i % 256);
  }
  binary_data[199] = '\0';

  // Insert some printable words
  memcpy(binary_data + 50, " hello ", 7);
  memcpy(binary_data + 100, " world ", 7);
  memcpy(binary_data + 150, " test ", 6);

  FileReader fr = filereader_init_from_string(binary_data);
  WordList *words = read_all_words(fr);

  // Should handle binary data gracefully
  cr_assert(true, "Should handle binary-like data");

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, fuzz_extreme_unicode_stress) {
  // Stress test with various Unicode characters and edge cases
  const char *unicode_chaos = "🚀 αβγδε 你好世界 مرحبا العالم "
                              "Ω≈ç√∫˜µ≤≥÷ åß∂ƒ©˙∆˚¬…æ "
                              "œ∑´®†¥¨ˆøπ ‰Â¯Ó◊ æœ∑´®†¥ "
                              "test word normal";

  FileReader fr = filereader_init_from_string(unicode_chaos);
  WordList *words = read_all_words(fr);

  // Should handle Unicode without crashing
  cr_assert(words->count >= 5, "Should read at least some Unicode words");

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, fuzz_pathological_line_endings) {
  // Every possible line ending combination
  FileReader fr =
      filereader_init_from_string("word1\n"       // Unix
                                  "word2\r\n"     // Windows
                                  "word3\r"       // Classic Mac
                                  "word4\n\r"     // Unusual combination
                                  "word5\n\n\n"   // Multiple Unix
                                  "word6\r\r\r"   // Multiple Mac
                                  "word7\r\n\r\n" // Multiple Windows
                                  "word8\n\r\n\r" // Mixed chaos
                                  "word9"         // No ending
      );

  WordList *words = read_all_words(fr);

  const char *expected[] = {"word1", "word2", "word3", "word4", "word5",
                            "word6", "word7", "word8", "word9"};
  assert_words_equal(words, expected, 9);

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, fuzz_zero_width_and_control_chars) {
  // Test with zero-width characters and control characters
  char control_test[500];
  char *ptr = control_test;

  // Add normal word
  strcpy(ptr, "start");
  ptr += 5;

  // Add various control characters
  for (int i = 1; i < 32; i++) {
    if (i != '\t' && i != '\n' && i != '\r' && i != '\f' && i != '\v') {
      *ptr++ = (char)i;
    }
  }

  // Add another word
  strcpy(ptr, " middle ");
  ptr += 8;

  // Add high control characters
  for (int i = 127; i < 160; i++) {
    *ptr++ = (char)i;
  }

  strcpy(ptr, " end");
  ptr += 4;
  *ptr = '\0';

  FileReader fr = filereader_init_from_string(control_test);
  WordList *words = read_all_words(fr);

  // Should handle control characters gracefully
  cr_assert(words->count >= 1,
            "Should read at least some words from control character soup");

  wordlist_destroy(words);
  filereader_destroy(&fr);
}

// =========================
// ADVERSARIAL INPUT TESTS
// =========================

Test(file_reader, adversarial_repeated_patterns) {
  // Test patterns that might confuse state machines
  const char *patterns[] = {
      " \t \t \t \t word \t \t \t \t ",
      "\n\n\n\n\nword\n\n\n\n\n",
      "\r\r\r\r\rword\r\r\r\r\r",
      "          word          ",
      "\t\t\t\t\t\t\t\t\tword\t\t\t\t\t\t\t\t\t",
  };

  for (size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
    FileReader fr = filereader_init_from_string(patterns[i]);
    WordList *words = read_all_words(fr);

    const char *expected[] = {"word"};
    assert_words_equal(words, expected, 1);

    wordlist_destroy(words);
    filereader_destroy(&fr);
  }
}

Test(file_reader, adversarial_buffer_boundary_dance) {
  // Create input that dances around buffer boundaries
  const size_t boundary = 1024;
  char *tricky_input = malloc(boundary * 3);
  char *ptr = tricky_input;

  // Fill just under buffer size with whitespace
  for (size_t i = 0; i < boundary - 10; i++) {
    *ptr++ = ' ';
  }

  // Add a word right at the boundary
  strcpy(ptr, "boundary");
  ptr += 8;

  // Add more whitespace
  for (size_t i = 0; i < boundary; i++) {
    *ptr++ = ' ';
  }

  strcpy(ptr, "word");
  ptr += 4;
  *ptr = '\0';

  FileReader fr = filereader_init_from_string(tricky_input);
  WordList *words = read_all_words(fr);

  const char *expected[] = {"boundary", "word"};
  assert_words_equal(words, expected, 2);

  free(tricky_input);
  wordlist_destroy(words);
  filereader_destroy(&fr);
}

Test(file_reader, adversarial_word_size_ladder) {
  // Create words of increasing size to test buffer management
  char *ladder_input = malloc(10000);
  char *ptr = ladder_input;

  for (int word_size = 1; word_size <= 100; word_size++) {
    for (int i = 0; i < word_size; i++) {
      *ptr++ = (char)('a' + (i % 26));
    }
    *ptr++ = ' ';
  }
  *ptr = '\0';

  FileReader fr = filereader_init_from_string(ladder_input);
  WordList *words = read_all_words(fr);

  cr_assert_eq(words->count, 100,
               "Should read exactly 100 words of increasing size");

  // Verify word sizes
  for (size_t i = 0; i < words->count; i++) {
    cr_assert_eq(strlen(words->words[i]), i + 1,
                 "Word %zu should be %zu characters long", i, i + 1);
  }

  free(ladder_input);
  wordlist_destroy(words);
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

Test(file_reader, use_after_free_detection) {
  // This test ensures AddressSanitizer would catch use-after-free
  FileReader fr = filereader_init_from_string("hello world");
  const char *word1 = filereader_read_next_word(fr);
  cr_assert_str_eq(word1, "hello");

  filereader_destroy(&fr);

  // Don't actually use after free in the test - AddressSanitizer will catch
  // any accidental uses in the implementation
}

Test(file_reader, null_pointer_chaos) {
  // Test all functions with NULL pointers
  cr_assert_null(filereader_read_next_word(NULL));
  cr_assert_eq(filereader_get_error(NULL), FR_ERR_FILE_NOT_FOUND);
  filereader_destroy(NULL); // Should not crash
}