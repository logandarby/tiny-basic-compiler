
// ------------------------------------
// ARGUMENT PARSING
// ------------------------------------

#include "args.h"
#include "core.h"
#include "dz_debug.h"
#include <string.h>

typedef enum ARG_TYPE {
  ARG_TYPE_NONE,
  ARG_TYPE_FILENAME,
  ARG_TYPE_LITERAL,
} ARG_TYPE;

static const char *FLAG_CODE = "-c";

struct Args {
  ARG_TYPE argType;
  union {
    const char *filename;
    const char *code_literal;
  };
};

enum ARG_ERROR {
  ARG_ERR_NONE = 0,
  ARG_ERR_INCORRECT_ARGS,
  ARG_ERR_BAD_CODE_STR,
};

const char *ARG_ERR_MSGS[] = {
    [ARG_ERR_NONE] = "",
    [ARG_ERR_INCORRECT_ARGS] = "An incorrect number of arguments were "
                               "supplied. Please check the usage.",
    [ARG_ERR_BAD_CODE_STR] =
        "You supplied a bad string as an argument. Please supply either the "
        "name of a file, or a string literal containing code."};

void print_usage(void);
void panic_with_error(enum ARG_ERROR error);

bool is_flag(const char *str) { return strlen(str) > 0 && str[0] == '-'; }

Args parse_args(const int argc, const char **argv) {
  if (argc < 2 || argc > 3) {
    panic_with_error(ARG_ERR_INCORRECT_ARGS);
  }
  // If 2 args, then must be filename
  if (argc == 2) {
    if (is_flag(argv[1])) {
      panic_with_error(ARG_ERR_INCORRECT_ARGS);
    }
    struct Args *return_value = malloc(sizeof(struct Args));
    *return_value =
        (struct Args){.argType = ARG_TYPE_FILENAME, .filename = argv[1]};
    return return_value;
  }
  // Must be 3 args
  if (strcmp(argv[1], FLAG_CODE) != 0) {
    panic_with_error(ARG_ERR_INCORRECT_ARGS);
  }
  // If its 3 args, then make sure the option flag is correct, and that the
  // proceeding code is literal code
  struct Args *return_value = malloc(sizeof(struct Args));
  *return_value =
      (struct Args){.argType = ARG_TYPE_LITERAL, .code_literal = argv[2]};
  return return_value;
}

void print_usage(void) {
  printf("\n");
  printf("Usage:\n");
  printf("teeny [-c code] [filename]\n\n");
  printf("  -c code    Compile literal code string\n");
  printf("  filename   Compile from file (default)\n");
  printf("\n");
}

void panic_with_error(const enum ARG_ERROR error) NORETURN;
void panic_with_error(const enum ARG_ERROR error) {
  DZ_ASSERT(error != ARG_ERR_NONE);
  fprintf(stderr, "%s\n", ARG_ERR_MSGS[error]);
  print_usage();
  exit(EXIT_FAILURE);
}

void args_free(Args *args_ptr) {
  if (!args_ptr || !(*args_ptr)) {
    return;
  }
  free(*args_ptr);
  *args_ptr = NULL;
}

// Depending on the arg type, can either get the filename, or the code literal.
// If you try and get the wrong kind of argument, the code will panic.
const char *args_get_filename(Args args) {
  DZ_ASSERT(args->argType == ARG_TYPE_FILENAME);
  return args->filename;
}
// Depending on the arg type, can either get the filename, or the code literal.
// If you try and get the wrong kind of argument, the code will panic.
const char *args_get_code_literal(Args args) {
  DZ_ASSERT(args->argType == ARG_TYPE_LITERAL);
  return args->code_literal;
}

bool args_is_filename(Args args) { return args->argType == ARG_TYPE_FILENAME; }

bool args_is_code_literal(Args args) {
  return args->argType == ARG_TYPE_LITERAL;
}

void args_print_usage(void) { print_usage(); }
