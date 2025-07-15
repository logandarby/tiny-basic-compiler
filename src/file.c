#include "file.h"
#include <stdio.h>
#include <string.h>

const size_t INIT_FILE_LINE_BUFFER = 1024;
const size_t WORD_BUFFER_SIZE = 1024;
const char* WHITESPACE = " \t\n\r\f\v\0";

typedef struct FileReaderHandle {
  FILE *file;
  const char *filename;
  char *line_buffer; // Dynamically allocated
  size_t current_line_length;
  char *current_word_ptr;
  char *current_word; // Dynamically allocated copy
  size_t line_buffer_size;
  enum FR_ERROR error; // Internal error state
} FileReaderHandle;

bool filereader_is_eof(FileReader fr) {
  if (!fr) {
    return true;
  }
  return feof(fr->file);
}

// Reads the next line into the line buffer, and sets the
// current word pointer to the start of the newline
// If the file reaches the EOF, then both these are set to NULL
void read_next_line(FileReader fr) {
  if (filereader_is_eof(fr)) {
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
  }
  fr->current_line_length = (size_t)bytes_read;
  fr->current_word_ptr = fr->line_buffer;
}

FileReader filereader_init(const char *filename) {
  FILE *fptr = fopen(filename, "r");
  if (!fptr) {
    return NULL;
  }
  FileReader fr = (FileReader)xmalloc(sizeof(FileReaderHandle));
  fr->line_buffer = (char *)xmalloc(INIT_FILE_LINE_BUFFER);
  fr->current_word = (char *)xmalloc(WORD_BUFFER_SIZE);
  fr->current_word_ptr = fr->line_buffer;
  fr->file = fptr;
  fr->error = FR_ERR_NONE;
  read_next_line(fr);
  return fr;
}

bool filereader_has_word(FileReader fr) {
  if (!fr) {
    return false;
  }
  return fr->current_word_ptr != NULL;
}

enum FR_ERROR filereader_get_error(FileReader fr) {
  if (!fr) {
    return FR_ERR_FILE_NOT_FOUND;
  }
  return fr->error;
}

void filereader_seek_word(FileReader fr) {
  if (!fr || filereader_is_eof(fr)) {
    return;
  }
  
  // Clear any previous errors
  fr->error = FR_ERR_NONE;
  
  // Keep looking for a word, potentially across multiple lines
  while (true) {
    if (!fr->current_word_ptr) {
      return;
    }

    // Skip leading whitespace on current line
    size_t whitespace_len = strspn(fr->current_word_ptr, WHITESPACE);
    fr->current_word_ptr += whitespace_len;
    
    // Find the length of the next word (stops at any whitespace)
    size_t word_len = strcspn(fr->current_word_ptr, WHITESPACE);
    
    // If we found a word, copy it and return
    if (word_len > 0) {
      // Check if word is too big for the buffer
      if (word_len >= WORD_BUFFER_SIZE) {
        fr->error = FR_ERR_WORD_TOO_BIG;
        return;
      }
      
      // Copy the word into current_word buffer
      strncpy(fr->current_word, fr->current_word_ptr, word_len);
      fr->current_word[word_len] = '\0';
      
      // Move the pointer past the current word
      fr->current_word_ptr += word_len;
      return;
    }
    
    // No word found on current line, try to read the next line
    read_next_line(fr);
    
    // If we can't read or reached EOF, exit (error already stored in fr->error by read_next_line)
    if (fr->error != FR_ERR_NONE || fr->current_word_ptr == NULL) {
      return;
    }
  }
}

const char* filereader_get_current_word(FileReader fr) {
  return fr->current_word;
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
  fclose(fr->file);
  free(fr);
}
