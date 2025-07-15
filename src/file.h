#pragma once

// --------------------------------------
// FILE READING UTILITY
//
// Utility class for the Lexer to read files 
// in an efficient manner
//
// Ignores spaces and tabs, but not newlines
// --------------------------------------

#include "core.h"

enum FR_ERROR {
  FR_ERR_NONE,
  FR_ERR_FILE_NOT_FOUND,
  FR_ERR_CANT_READ,
  FR_ERR_LINE_TOO_BIG,
  FR_ERR_WORD_TOO_BIG,
};

// Opaque type for file reading
typedef struct FileReaderHandle* FileReader;

// Initializes a FileReader pointing to the file specified in the filename
FileReader filereader_init(const char* filename);

// Returns true if there are still words to read
bool filereader_has_word(FileReader fr);

// Returns the current error state
enum FR_ERROR filereader_get_error(FileReader fr);

// Seeks the file reader to the beginning of the next word
void filereader_seek_word(FileReader fr);

// Returns the current word
const char* filereader_get_current_word(FileReader fr);

// Frees and closes any files associated with the file reader
void filereader_destroy(FileReader fr);

