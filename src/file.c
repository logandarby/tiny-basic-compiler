#include "file.h"
#include <stdio.h>
#include <string.h>

const size_t INIT_FILE_LINE_BUFFER = 1024;
const size_t WORD_BUFFER_SIZE = 1024;
const char *WHITESPACE = " \t\n\r\f\v";

typedef struct FileReaderHandle {
  FILE *file;
  const char *filename;
  char *line_buffer; // Dynamically allocated
  size_t current_line_length;
  char *current_word_ptr;
  char *current_word; // Dynamically allocated copy
  size_t line_buffer_size;
  enum FR_ERROR error; // Internal error state
  // Holds a copy of the input string when the reader is backed by an in-memory
  // buffer created via fmemopen. This is freed during destruction. NULL when
  // reading from an actual file.
  char *string_buffer;
} FileReaderHandle;

bool _filereader_is_eof(const FileReader fr) {
  if (!fr) {
    return true;
  }
  return feof(fr->file);
}

void _filereader_debug_print(const FileReader fr) {
  return;
  printf("FileReader: %p\n", fr);
  printf("File: %s\n", fr->filename);
  printf("Line buffer: %s\n", fr->line_buffer);
  printf("Current word: %s\n", fr->current_word);
  printf("Current word ptr: %p\n", fr->current_word_ptr);
  printf("Current line length: %zu\n", fr->current_line_length);
  printf("Line buffer size: %zu\n", fr->line_buffer_size);
  printf("Error: %d\n", fr->error);
  printf("Is EOF: %s\n", _filereader_is_eof(fr) ? "true" : "false");
}

// Reads the next line into the line buffer, and sets the
// current word pointer to the start of the newline
// If the file reaches the EOF, then both these are set to NULL
void _read_next_line(FileReader fr) {
  if (_filereader_is_eof(fr)) {
    fr->current_word_ptr = NULL;
    fr->current_line_length = 0;
    return;
  }
  const ssize_t bytes_read =
      getline(&fr->line_buffer, &fr->line_buffer_size, fr->file);
  if (bytes_read == -1 && !feof(fr->file)) {
    fr->error = FR_ERR_CANT_READ;
    fprintf(stderr, "CRITICAL: Could not read next line of file: %s\n",
            strerror(errno));
    exit(EXIT_FAILURE);
    return;
  } else if (bytes_read == -1) {
    fr->current_line_length = 0;
    fr->current_word_ptr = NULL;
    return;
  }
  fr->current_line_length =
      bytes_read == -1 ? strlen(fr->line_buffer) : (size_t)bytes_read;
  fr->current_word_ptr = fr->line_buffer;
}

FileReader filereader_init(const char *filename) {
  FILE *fptr = fopen(filename, "r");
  if (!fptr) {
    return NULL;
  }
  FileReaderHandle fr = {
      .file = fptr,
      .filename = filename,
      .line_buffer = (char *)xcalloc(1, INIT_FILE_LINE_BUFFER),
      .current_word = (char *)xcalloc(1, WORD_BUFFER_SIZE),
      .error = FR_ERR_NONE,
      .current_word_ptr = NULL,
      .current_line_length = 0,
      .line_buffer_size = INIT_FILE_LINE_BUFFER,
      .string_buffer = NULL,
  };
  FileReader return_val = (FileReader)xmalloc(sizeof(FileReaderHandle));
  memcpy(return_val, &fr, sizeof(FileReaderHandle));
  _read_next_line(return_val);
  _filereader_debug_print(return_val);
  return return_val;
}

bool filereader_has_word(const FileReader fr) {
  if (!fr || fr->error) {
    return false;
  }
  return !feof(fr->file) ||
         (fr->current_word_ptr != NULL && *fr->current_word_ptr != '\0');
}

enum FR_ERROR filereader_get_error(FileReader fr) {
  if (!fr) {
    return FR_ERR_FILE_NOT_FOUND;
  }
  return fr->error;
}

const char *filereader_read_next_word(FileReader fr) {
  while (true) {
    _filereader_debug_print(fr);
    if (!filereader_has_word(fr)) {
      return NULL;
    }
    if (fr->current_word_ptr == NULL || *fr->current_word_ptr == '\0' ||
        *fr->current_word_ptr == '\n') {
      _read_next_line(fr);
      continue;
    }
    // Skip Initial Whitespace
    const size_t whitespace_len = strspn(fr->current_word_ptr, WHITESPACE);
    fr->current_word_ptr += whitespace_len;
    // If reaches end of line, continue
    if (*fr->current_word_ptr == '\0' || *fr->current_word_ptr == '\n') {
      _read_next_line(fr);
      continue;
    }
    // Find the length of the word, copy into buffer, and seek the current word
    // ptr
    const size_t word_len = strcspn(fr->current_word_ptr, WHITESPACE);
    if (word_len + 1 > WORD_BUFFER_SIZE) {
      fr->error = FR_ERR_WORD_TOO_BIG;
      return NULL;
    }
    strncpy(fr->current_word, fr->current_word_ptr, word_len);
    fr->current_word[word_len] = '\0';
    fr->current_word_ptr += word_len;
    return fr->current_word;
  }
}

void filereader_destroy(FileReader fr) {
  if (!fr) {
    return;
  }
  if (fr->line_buffer) {
    free(fr->line_buffer);
  }
  if (fr->current_word) {
    free(fr->current_word);
  }
  if (fr->string_buffer) {
    // Close the FILE* before freeing the backing buffer to avoid undefined
    // behaviour with fmemopen.
    fclose(fr->file);
    free(fr->string_buffer);
  } else {
    fclose(fr->file);
  }
  free(fr);
}

// --------------------------------------
// PUBLIC API: Initialize from in-memory string
// --------------------------------------
FileReader filereader_init_from_string(const char *input) {
  if (!input) {
    return NULL;
  }

  // Make a private, mutable copy because fmemopen expects a writeable buffer
  const size_t len = strlen(input);
  char *buffer = (char *)xmalloc(len + 1);
  memcpy(buffer, input, len + 1);

  FILE *stream = fmemopen(buffer, len + 1, "r");
  if (!stream) {
    free(buffer);
    return NULL;
  }

  FileReaderHandle fr = {
      .file = stream,
      .filename = "<memory>",
      .line_buffer = (char *)xcalloc(1, INIT_FILE_LINE_BUFFER),
      .current_word = (char *)xcalloc(1, WORD_BUFFER_SIZE),
      .error = FR_ERR_NONE,
      .current_word_ptr = NULL,
      .current_line_length = 0,
      .line_buffer_size = INIT_FILE_LINE_BUFFER,
      .string_buffer = buffer,
  };

  FileReader return_val = (FileReader)xmalloc(sizeof(FileReaderHandle));
  memcpy(return_val, &fr, sizeof(FileReaderHandle));
  _read_next_line(return_val);
  _filereader_debug_print(return_val);
  return return_val;
}
