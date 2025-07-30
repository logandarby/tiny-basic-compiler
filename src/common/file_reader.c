#include "file_reader.h"
#include "../debug/dz_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const size_t INIT_FILE_LINE_BUFFER = 1024;

typedef struct FileReaderHandle {
  FileIO *io; // File I/O abstraction layer
  const char *filename;
  char *line_buffer; // Dynamically allocated
  size_t current_line_length;
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
  UNUSED(fr);
  return;
  DZ_INFO("FileReader: %p\n", fr);
  DZ_INFO("File: %s\n", fr->io->label);
  DZ_INFO("Line buffer: %s\n", fr->line_buffer);
  DZ_INFO("Current line length: %zu\n", fr->current_line_length);
  DZ_INFO("Line buffer size: %zu\n", fr->line_buffer_size);
  DZ_INFO("Error: %d\n", fr->error);
  DZ_INFO("Is EOF: %s\n", _filereader_is_eof(fr) ? "true" : "false");
}

// Reads the next line into the line buffer, and sets the
// current word pointer to the start of the newline
// If the file reaches the EOF, then both these are set to NULL
bool _read_next_line(FileReader fr) {
  if (_filereader_is_eof(fr)) {
    fr->line_buffer[0] = '\0';
    fr->current_line_length = 0;
    return false;
  }
  const ssize_t bytes_read =
      fr->io->getline(&fr->line_buffer, &fr->line_buffer_size, fr->io->stream);
  if (bytes_read == -1 && !fr->io->feof(fr->io->stream)) {
    fr->error = FR_ERR_CANT_READ;
    DZ_ERRORNO("CRITICAL: Could not read next line of file %s\n", fr->filename);
    exit(EXIT_FAILURE);
    return false;
  } else if (bytes_read == -1) {
    fr->line_buffer[0] = '\0';
    fr->current_line_length = 0;
    return true;
  }
  fr->current_line_length =
      bytes_read == -1 ? strlen(fr->line_buffer) : (size_t)bytes_read;
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
      .line_buffer = (char *)xcalloc(1, INIT_FILE_LINE_BUFFER),
      .error = FR_ERR_NONE,
      .current_line_length = 0,
      .line_buffer_size = INIT_FILE_LINE_BUFFER,
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
    return NULL;
  }

  return fr->line_buffer;
}

void filereader_destroy(FileReader *fr) {
  if (!fr || !*fr) {
    return;
  }

  FileReader handle = *fr;

  if (handle->line_buffer) {
    free(handle->line_buffer);
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

FileReader filereader_init_from_fileio(FileIO *io) {
  if (!io) {
    return NULL;
  }

  FileReaderHandle fr = {
      .io = io,
      .filename = io->label,
      .line_buffer = (char *)xcalloc(1, INIT_FILE_LINE_BUFFER),
      .error = FR_ERR_NONE,
      .current_line_length = 0,
      .line_buffer_size = INIT_FILE_LINE_BUFFER,
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
  return fr->line_buffer_size;
}
