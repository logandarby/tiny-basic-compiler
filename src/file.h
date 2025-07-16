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

enum FR_ERROR {
  FR_ERR_NONE,
  FR_ERR_FILE_NOT_FOUND,
  FR_ERR_CANT_READ,    // Read operation wasn't performed.
  FR_ERR_LINE_TOO_BIG, // Line was too big to fit into a buffer
  FR_ERR_WORD_TOO_BIG, // Word was too big to fit into a buffer
};

// Opaque type for file reading
typedef struct FileReaderHandle *FileReader;

// Initializes a FileReader pointing to the file specified in the filename
FileReader filereader_init(const char *filename);

// Initializes a FileReader from a null-terminated string buffer held in memory
// The content of the string is copied internally, so the caller does not need
// to keep the original buffer alive for the lifetime of the FileReader.
FileReader filereader_init_from_string(const char *input);

// Returns the current error state
enum FR_ERROR filereader_get_error(const FileReader fr);

// Reads the next word of the file. If the file hits the EOF, returns NULL
const char *filereader_read_next_word(FileReader fr);

// Returns a pointer to the current word. Does not modify the struct
const char *filereader_get_current_word(const FileReader fr);

// Frees and closes any files associated with the file reader
void filereader_destroy(FileReader fr);
