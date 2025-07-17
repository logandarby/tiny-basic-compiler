#pragma once

// --------------------------------------
// FILE READING UTILITY
//
// Utility class for the Lexer to read files word by word
// in an efficient manner
//
// Ignores spaces and tabs, but not newlines
// --------------------------------------

#include "core.h"

// --------------------------------------
// FILE I/O ABSTRACTION LAYER
//
// Allows dependency injection for testing and mocking file operations
// --------------------------------------

// Forward declarations
typedef struct FileIO FileIO;
typedef struct FileReaderHandle *FileReader;

// File I/O function pointer types
typedef ssize_t (*getline_fn)(char **lineptr, size_t *n, void *stream);
typedef int (*feof_fn)(void *stream);
typedef int (*fclose_fn)(void *stream);
typedef void (*cleanup_fn)(void *stream);

// File I/O abstraction interface
struct FileIO {
  void *stream;       // Actual FILE*, memory buffer, mock object, etc.
  const char *label;  // Human-readable identifier for debugging
  getline_fn getline; // Function to read a line
  feof_fn feof;       // Function to check end-of-file
  fclose_fn fclose;   // Function to close the stream
  cleanup_fn cleanup; // Optional custom cleanup function
  void *cleanup_data; // Data passed to cleanup function
};

// Standard FILE* implementations
extern const FileIO *fileio_stdio;

// Creates a FileIO wrapper around a standard FILE*
FileIO *fileio_create_stdio(FILE *stream, const char *label);

// Creates a FileIO wrapper from an in-memory string (using fmemopen)
FileIO *fileio_create_from_string(const char *input, const char *label);

// Destroys a FileIO wrapper
void fileio_destroy(FileIO *io);

// --------------------------------------
// FILE READER API
// --------------------------------------

enum FR_ERROR {
  FR_ERR_NONE,
  FR_ERR_FILE_NOT_FOUND,
  FR_ERR_CANT_READ,    // Read operation wasn't performed.
  FR_ERR_LINE_TOO_BIG, // Line was too big to fit into a buffer
  FR_ERR_WORD_TOO_BIG, // Word was too big to fit into a buffer
};

// Opaque type for file reading
// typedef struct FileReaderHandle *FileReader; // This line is now redundant

// Initializes a FileReader pointing to the file specified in the filename
FileReader filereader_init(const char *filename);

// Initializes a FileReader from a null-terminated string buffer held in memory
// The content of the string is copied internally, so the caller does not need
// to keep the original buffer alive for the lifetime of the FileReader.
FileReader filereader_init_from_string(const char *input);

// Initializes a FileReader using dependency injection with a FileIO abstraction
// Takes ownership of the FileIO object and will destroy it when the FileReader
// is destroyed
FileReader filereader_init_from_fileio(FileIO *io);

// Returns the current error state
enum FR_ERROR filereader_get_error(const FileReader fr);

// Reads the next line of the file. If the file hits the EOF, returns NULL
const char *filereader_read_next_line(FileReader fr);

// Returns a pointer to the current line. Does not modify the struct
const char *filereader_get_current_line(const FileReader fr);

size_t filereader_get_linebuffer_length(const FileReader fr);

// Frees and closes any files associated with the file reader
// Sets the pointer to NULL to prevent double-free
void filereader_destroy(FileReader *fr);
