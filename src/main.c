// -------------------------------------
// TEENY TINY BASIC COMPILER
//
// Logan Darby
//
// A compiler for the TINY Basic Language
// specification, with some added features for good
// measure.
// -------------------------------------

#include "ast/ast_utils.h"
#include "common/error_reporter.h"
#include "common/file_reader.h"
#include "core/args.h"
#include "frontend/lexer/lexer.h"
#include "frontend/parser/parser.h"

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

  AST ast = ast_parse(tokens);
  ast_print(&ast);

  ast_destroy(&ast);
  token_array_destroy(&tokens);

  if (er_has_errors()) {
    er_print_all_errors();
  }

  er_free();

  return EXIT_SUCCESS;
}
