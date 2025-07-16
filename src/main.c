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

// -------------------------------------
// LEXING
// -------------------------------------

enum TOKEN {
  // Operators
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_MULT,
  TOKEN_DIV,
  // Relation Operators
  TOKEN_GT,
  TOKEN_LT,
  TOKEN_GTE,
  TOKEN_LTE,
  TOKEN_EQ,
  TOKEN_EQEQ,
  TOKEN_NOTEQ,
  // Literals
  TOKEN_STRING,
  TOKEN_NUMBER,
  TOKEN_ENDOFFILE,
  TOKEN_NEWLINE,
  TOKEN_IDENT,
  // Internal Keywords
  TOKEN_LABEL,
  TOKEN_GOTO,
  TOKEN_PRINT,
  TOKEN_INPUT,
  TOKEN_LET,
  TOKEN_IF,
  TOKEN_THEN,
  TOKEN_ELSE,
  TOKEN_ENDIF,
  TOKEN_WHILE,
  TOKEN_REPEAT,
  TOKEN_ENDWHILE,
};

int main(const int argc, const char **argv) {
  const struct Args args = parse_args(argc, argv);
  const char *filename = args.filename;
  FileReader fr = filereader_init_from_string("Testing\n1231    23\n  sdf");

  if (!fr) {
    fprintf(stderr, "File %s not found. Error: %s\n", filename,
            strerror(errno));
    return EXIT_FAILURE;
  }

  const char *word;
  while ((word = filereader_read_next_word(fr))) {
    printf("%s\n", word);
  }

  filereader_destroy(fr);

  return EXIT_SUCCESS;
}
