#include "system.h"

// Generate a named temporary file
FILE *create_named_tmpfile(char *filepath, size_t filepath_size) {
#if defined(_WIN32) || defined(_WIN64)
  char temp_dir[PATH_MAX];
  char temp_filename[PATH_MAX];
  // Get Windows temp directory
  if (GetTempPathA(sizeof(temp_dir), temp_dir) == 0) {
    strcpy(temp_dir, ".");
  }
  // Generate unique filename
  char prefix[4];
  snprintf(prefix, sizeof(prefix), "cc_");
  if (GetTempFileNameA(temp_dir, prefix, 0, temp_filename) == 0) {
    return NULL;
  }
  strncpy(filepath, temp_filename, filepath_size - 1);
  filepath[filepath_size - 1] = '\0';
  // Change extension to .s for assembly
  char *ext = strrchr(filepath, '.');
  if (ext)
    strcpy(ext, ".s");
  else
    strcat(filepath, ".s");
  return fopen(filepath, "w");
#else
  // Unix/Linux/macOS
  strncpy(filepath, "/tmp/compiler_XXXXXX.s", filepath_size - 1);
  filepath[filepath_size - 1] = '\0';
  int fd = mkstemps(filepath, 2); // 2 = length of ".s" suffix
  if (fd == -1)
    return NULL;
  return fdopen(fd, "w");
#endif
}
