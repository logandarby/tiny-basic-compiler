#include "file_reader.h"
#include "../debug/dz_debug.h"
#include "error_reporter.h"
#include "string_util.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_BUFFER 1000

typedef struct FileReaderHandle {
  FileIO *io; // File I/O abstraction layer
  const char *filename;
  char line_buffer[MAX_LINE_BUFFER]; // Fixed size buffer
  size_t current_line_length;
  // Publically available struct of current line number to create better error
  // msgs 1 - Indexed
  struct {
    size_t line;
  } cursor_pos;
  enum FR_ERROR error; // Internal error state
} FileReaderHandle;

// --------------------------------------
// FILE I/O ABSTRACTION IMPLEMENTATIONS
// --------------------------------------

// Standard stdio wrappers
static char *_stdio_fgets(char *buffer, int size, void *stream) {
  return fgets(buffer, size, (FILE *)stream);
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
  io->fgets = _stdio_fgets;
  io->feof = _stdio_feof;
  io->fclose = _stdio_fclose;
  io->cleanup = NULL;
  io->cleanup_data = NULL;
  return io;
}

FileIO *fileio_create_from_string(const char *input, const char *label) {
  if (!input || !label) {
    return NULL;
  }

  // Create temporary file and write string data to it
  FILE *stream = tmpfile();
  if (!stream) {
    return NULL;
  }

  const size_t len = strlen(input);
  if (fwrite(input, 1, len, stream) != len) {
    fclose(stream);
    return NULL;
  }

  // Rewind to beginning for reading
  rewind(stream);

  FileIO *io = fileio_create_stdio(stream, label);
  if (!io) {
    fclose(stream);
    return NULL;
  }

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
  UNUSED(fr);
  return;
  DZ_INFO("FileReader: %p\n", fr);
  DZ_INFO("File: %s\n", fr->io->label);
  DZ_INFO("Line buffer: %s\n", fr->line_buffer);
  DZ_INFO("Current line length: %zu\n", fr->current_line_length);
  DZ_INFO("Error: %d\n", fr->error);
  DZ_INFO("Is EOF: %s\n", _filereader_is_eof(fr) ? "true" : "false");
}

// Reads the next line into the line buffer, and sets the
// current word pointer to the start of the newline
// If the file reaches the EOF, then both these are set to NULL
// Strips the newlines at the end.
bool _read_next_line(FileReader fr) {
  if (_filereader_is_eof(fr)) {
    fr->line_buffer[0] = '\0';
    fr->current_line_length = 0;
    return false;
  }

  char *result =
      fr->io->fgets(fr->line_buffer, MAX_LINE_BUFFER, fr->io->stream);
  if (result == NULL) {
    if (fr->io->feof(fr->io->stream)) {
      fr->line_buffer[0] = '\0';
      fr->current_line_length = 0;
      return true;
    } else {
      fr->error = FR_ERR_CANT_READ;
      DZ_ERRORNO("CRITICAL: Could not read next line of file %s\n",
                 fr->filename);
      exit(EXIT_FAILURE);
      return false;
    }
  }

  // Check if line was truncated (buffer too small)
  size_t line_len = strlen(fr->line_buffer);
  if (line_len == MAX_LINE_BUFFER - 1 &&
      fr->line_buffer[line_len - 1] != '\n' && !fr->io->feof(fr->io->stream)) {
    fr->error = FR_ERR_LINE_TOO_BIG;
    size_t next_line =
        fr->cursor_pos.line == NO_LINE_NUMBER ? 1 : fr->cursor_pos.line + 1;
    DZ_ERRORNO("CRITICAL: Line too long in file %s at line %lld\nPlease try to "
               "keep lines ~100 characters.",
               fr->filename, (unsigned long long)next_line);
    exit(EXIT_FAILURE);
    return false;
  }

  strip_trailing_newlines(fr->line_buffer, MAX_LINE_BUFFER - 1);
  fr->current_line_length = strlen(fr->line_buffer);
  return true;
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
      .line_buffer = {0}, // Initialize fixed buffer to zero
      .error = FR_ERR_NONE,
      .current_line_length = 0,
      .cursor_pos =
          {
              .line = NO_LINE_NUMBER,
          },
  };
  FileReader return_val = (FileReader)xmalloc(sizeof(FileReaderHandle));
  memcpy(return_val, &fr, sizeof(FileReaderHandle));
  _filereader_debug_print(return_val);
  return return_val;
}

enum FR_ERROR filereader_get_error(FileReader fr) {
  if (!fr) {
    return FR_ERR_FILE_NOT_FOUND;
  }
  return fr->error;
}

const char *filereader_read_next_line(FileReader fr) {
  if (!fr) {
    return NULL;
  }

  const bool success = _read_next_line(fr);
  if (!success) {
    fr->cursor_pos.line = NO_LINE_NUMBER;
    return NULL;
  }

  // Move current cursor
  if (fr->cursor_pos.line == NO_LINE_NUMBER) {
    fr->cursor_pos.line = 1;
  } else {
    fr->cursor_pos.line++;
  }

  return fr->line_buffer;
}

void filereader_destroy(FileReader *fr) {
  if (!fr || !*fr) {
    return;
  }

  FileReader handle = *fr;

  // No need to free line_buffer since it's a fixed array now
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

FileReader filereader_init_from_fileio(FileIO *io) {
  if (!io) {
    return NULL;
  }

  FileReaderHandle fr = {
      .io = io,
      .filename = io->label,
      .line_buffer = {0}, // Initialize fixed buffer to zero
      .error = FR_ERR_NONE,
      .current_line_length = 0,
      .cursor_pos =
          {
              .line = NO_LINE_NUMBER,
          },
  };

  FileReader return_val = (FileReader)xmalloc(sizeof(FileReaderHandle));
  memcpy(return_val, &fr, sizeof(FileReaderHandle));
  _filereader_debug_print(return_val);
  return return_val;
}

const char *filereader_get_current_line(FileReader fr) {
  return fr->line_buffer;
}

size_t filereader_get_linebuffer_length(const FileReader fr) {
  UNUSED(fr);
  return MAX_LINE_BUFFER;
}

size_t filereader_get_current_line_number(FileReader fr) {
  return fr->cursor_pos.line;
}

const char *filereader_get_filename_ref(const FileReader fr) {
  return fr->filename;
}
