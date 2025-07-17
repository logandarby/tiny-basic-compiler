#include "file.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const size_t INIT_FILE_LINE_BUFFER = 1024;
const size_t WORD_BUFFER_SIZE = 1024;
const char *WHITESPACE = " \t\n\r\f\v";

typedef struct FileReaderHandle {
  FileIO *io; // File I/O abstraction layer
  const char *filename;
  char *line_buffer; // Dynamically allocated
  size_t current_line_length;
  char *current_word_ptr;
  char *current_word; // Dynamically allocated copy
  size_t line_buffer_size;
  enum FR_ERROR error; // Internal error state
} FileReaderHandle;

// --------------------------------------
// FILE I/O ABSTRACTION IMPLEMENTATIONS
// --------------------------------------

// Standard stdio wrappers
static ssize_t _stdio_getline(char **lineptr, size_t *n, void *stream) {
  return getline(lineptr, n, (FILE *)stream);
}

static int _stdio_feof(void *stream) { return feof((FILE *)stream); }

static int _stdio_fclose(void *stream) { return fclose((FILE *)stream); }

FileIO *fileio_create_stdio(FILE *stream, const char *label) {
  if (!stream || !label) {
    return NULL;
  }

  FileIO *io = (FileIO *)xmalloc(sizeof(FileIO));
  io->stream = stream;
  io->label = label;
  io->getline = _stdio_getline;
  io->feof = _stdio_feof;
  io->fclose = _stdio_fclose;
  io->cleanup = NULL;
  io->cleanup_data = NULL;
  return io;
}

// Custom cleanup function for string-based streams
static void _string_cleanup(void *cleanup_data) {
  free(cleanup_data); // Free the allocated string buffer
}

FileIO *fileio_create_from_string(const char *input, const char *label) {
  if (!input || !label) {
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

  FileIO *io = fileio_create_stdio(stream, label);
  if (!io) {
    fclose(stream);
    free(buffer);
    return NULL;
  }

  // Set up custom cleanup to free the string buffer
  io->cleanup = _string_cleanup;
  io->cleanup_data = buffer;
  return io;
}

void fileio_destroy(FileIO *io) {
  if (!io) {
    return;
  }
  if (io->fclose && io->stream) {
    io->fclose(io->stream);
  }
  if (io->cleanup && io->cleanup_data) {
    io->cleanup(io->cleanup_data);
  }
  free(io);
}

// --------------------------------------
// FILE READER IMPLEMENTATIONS
// --------------------------------------

bool _filereader_is_eof(const FileReader fr) {
  if (!fr) {
    return true;
  }
  return fr->io->feof(fr->io->stream);
}

void _filereader_debug_print(const FileReader fr) {
  return;
  printf("FileReader: %p\n", fr);
  printf("File: %s\n", fr->io->label);
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
      fr->io->getline(&fr->line_buffer, &fr->line_buffer_size, fr->io->stream);
  if (bytes_read == -1 && !fr->io->feof(fr->io->stream)) {
    fr->error = FR_ERR_CANT_READ;
    fprintf(stderr, "CRITICAL: Could not read next line of file: %s\n",
            strerror(errno));
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

  FileIO *io = fileio_create_stdio(fptr, filename);
  if (!io) {
    fclose(fptr);
    return NULL;
  }

  FileReaderHandle fr = {
      .io = io,
      .filename = filename,
      .line_buffer = (char *)xcalloc(1, INIT_FILE_LINE_BUFFER),
      .current_word = (char *)xcalloc(1, WORD_BUFFER_SIZE),
      .error = FR_ERR_NONE,
      .current_word_ptr = NULL,
      .current_line_length = 0,
      .line_buffer_size = INIT_FILE_LINE_BUFFER,
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
  return !fr->io->feof(fr->io->stream) ||
         (fr->current_word_ptr != NULL && *fr->current_word_ptr != '\0');
}

enum FR_ERROR filereader_get_error(FileReader fr) {
  if (!fr) {
    return FR_ERR_FILE_NOT_FOUND;
  }
  return fr->error;
}

const char *filereader_read_next_word(FileReader fr) {
  if (!fr) {
    return NULL;
  }
  while (true) {
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

void filereader_destroy(FileReader *fr) {
  if (!fr || !*fr) {
    return;
  }

  FileReader handle = *fr;

  if (handle->line_buffer) {
    free(handle->line_buffer);
  }
  if (handle->current_word) {
    free(handle->current_word);
  }
  if (handle->io) {
    fileio_destroy(handle->io);
  }
  free(handle);

  // Set the original pointer to NULL to prevent double-free
  *fr = NULL;
}

FileReader filereader_init_from_string(const char *input) {
  if (!input) {
    return NULL;
  }

  FileIO *io = fileio_create_from_string(input, "<memory>");
  if (!io) {
    return NULL;
  }

  return filereader_init_from_fileio(io);
}

char filereader_peek_char(const FileReader fr) {
  if (!fr || fr->error) {
    return '\0';
  }

  // If we don't have a current line or we're at the end, try to read next line
  if (!fr->current_word_ptr || *fr->current_word_ptr == '\0') {
    if (_filereader_is_eof(fr)) {
      return '\0';
    }
    // We're at end of current line but not EOF, so there's a newline
    if (fr->current_word_ptr && fr->current_line_length > 0) {
      return '\n';
    }
  }

  return fr->current_word_ptr ? *fr->current_word_ptr : '\0';
}

char filereader_read_char(FileReader fr) {
  if (!fr || fr->error) {
    return '\0';
  }

  char current_char = filereader_peek_char(fr);

  if (current_char == '\0') {
    return '\0';
  }

  if (current_char == '\n' || !fr->current_word_ptr ||
      *fr->current_word_ptr == '\0') {
    // We're at end of line, advance to next line
    _read_next_line(fr);
    return current_char;
  }

  // Advance position within current line
  fr->current_word_ptr++;
  return current_char;
}

size_t filereader_get_position(const FileReader fr) {
  if (!fr || !fr->line_buffer || !fr->current_word_ptr) {
    return 0;
  }

  return (size_t)(fr->current_word_ptr - fr->line_buffer);
}

const char *filereader_get_line_from_position(const FileReader fr) {
  if (!fr || !fr->current_word_ptr) {
    return NULL;
  }

  return fr->current_word_ptr;
}

FileReader filereader_init_from_fileio(FileIO *io) {
  if (!io) {
    return NULL;
  }

  FileReaderHandle fr = {
      .io = io,
      .filename = io->label,
      .line_buffer = (char *)xcalloc(1, INIT_FILE_LINE_BUFFER),
      .current_word = (char *)xcalloc(1, WORD_BUFFER_SIZE),
      .error = FR_ERR_NONE,
      .current_word_ptr = NULL,
      .current_line_length = 0,
      .line_buffer_size = INIT_FILE_LINE_BUFFER,
  };

  FileReader return_val = (FileReader)xmalloc(sizeof(FileReaderHandle));
  memcpy(return_val, &fr, sizeof(FileReaderHandle));
  _read_next_line(return_val);
  _filereader_debug_print(return_val);
  return return_val;
}

const char *filereader_get_current_word(FileReader fr) {
  return fr->current_word_ptr;
}
