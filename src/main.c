// -------------------------------------
// TEENY TINY BASIC COMPILER
//
// Logan Darby
//
// A compiler for the TINY Basic Language
// specification, with some added features for good
// measure.
// -------------------------------------

#include "args.h"
#include "file.h"

int main(const int argc, const char **argv) {
  const struct Args args = parse_args(argc, argv);
  const char *filename = args.filename;
  FileReader fr = filereader_init(filename);

  if (!fr) {
    fprintf(stderr, "File %s not found. Error: %s\n", filename,
            strerror(errno));
    return EXIT_FAILURE;
  }

  const char *line;
  while ((line = filereader_read_next_line(fr))) {
    printf("%s\n", line);
  }

  filereader_destroy(&fr);

  return EXIT_SUCCESS;
}
