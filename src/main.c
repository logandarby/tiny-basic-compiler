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
#include "lexer.h"
#include "parser.h"

int main(const int argc, const char **argv) {
  const struct Args args = parse_args(argc, argv);
  const char *filename = args.filename;
  FileReader fr = filereader_init(filename);

  if (!fr) {
    fprintf(stderr, "File %s not found. Error: %s\n", filename,
            strerror(errno));
    return EXIT_FAILURE;
  }

  TokenArray tokens = lexer_parse(fr);
  filereader_destroy(&fr);

  AST ast = ast_init();
  ast_parse(&ast, tokens);

  ast_destroy(&ast);
  token_array_destroy(&tokens);

  return EXIT_SUCCESS;
}
