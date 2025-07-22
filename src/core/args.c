
// ------------------------------------
// ARGUMENT PARSING
// ------------------------------------

#include "args.h"
#include "core.h"

enum ARG_ERROR {
  ARG_ERR_NONE = 0,
  ARG_ERR_INCORRECT_ARGS,
};

const char *ARG_ERR_MSGS[] = {[ARG_ERR_NONE] = "",
                              [ARG_ERR_INCORRECT_ARGS] =
                                  "An incorrect number of arguments were "
                                  "supplied. Please check the usage."};

void print_usage(void);
void panic_with_error(enum ARG_ERROR error);

struct Args parse_args(const int argc, const char **argv) {
  if (argc != 2) {
    panic_with_error(ARG_ERR_INCORRECT_ARGS);
  }
  const char *filename = argv[1];
  struct Args return_value = {.filename = filename};
  return return_value;
}

void print_usage(void) {
  printf("Usage:\n");
  printf("\tteeny filename\n");
}

void panic_with_error(const enum ARG_ERROR error) {
  if (!error) {
    return;
  }
  fprintf(stderr, "%s\n", ARG_ERR_MSGS[error]);
  print_usage();
  exit(EXIT_FAILURE);
}
