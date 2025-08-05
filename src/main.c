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
#include "common/symbol_table.h"
#include "core/args.h"
#include "dz_debug.h"
#include "frontend/lexer/lexer.h"
#include "frontend/parser/parser.h"
#include <stb_ds.h>
#include <stdlib.h>

int main(const int argc, const char **argv) {
  static const char *SEP = "-------------------";
  Args args = parse_args(argc, argv);

  // Get the FileReader object depending on what the args have
  FileReader fr = NULL;
  if (args_is_filename(args)) {
    const char *filename = args_get_filename(args);
    fr = filereader_init(filename);
    if (!fr) {
      fprintf(stderr, "%s[ERROR]%s File %s\"%s\"%s not found. Error: %s\n",
              KRED, KNRM, KCYN, filename, KNRM, strerror(errno));
      args_print_usage();
      args_free(&args);
      return EXIT_FAILURE;
    }
  } else if (args_is_code_literal(args)) {
    fr = filereader_init_from_string(args_get_code_literal(args));
    if (!fr) {
      fprintf(stderr, "%s[ERROR]%s Invalid code literal provided.", KRED, KNRM);
      args_print_usage();
      args_free(&args);
      return EXIT_FAILURE;
    }
  }

  args_free(&args);

  TokenArray tokens = lexer_parse(fr);
  filereader_destroy(&fr);

  AST ast = ast_parse(tokens);
  printf("%s AST PRINT %s\n", SEP, SEP);
  ast_print(&ast);

  VariableTable *vars = variables_collect_from_ast(&ast);
  printf("%s SYMBOL TABLE %s\n", SEP, SEP);
  for (size_t i = 0; i < (size_t)shlen(vars->symbol_table); i++) {
    SymbolHash sym = vars->symbol_table[i];
    printf("Key: %s,\tValue: %ld\n", sym.key, sym.value.label);
  }
  printf("%s LITERAL TABLE %s\n", SEP, SEP);
  for (size_t i = 0; i < (size_t)shlen(vars->literal_table); i++) {
    LiteralHash lit = vars->literal_table[i];
    printf("Key: %s,\tValue: %ld\n", lit.key, lit.value.label);
  }
  UNUSED(vars);
  printf("%s END OUTPUT %s\n", SEP, SEP);

  variables_destroy(vars);
  ast_destroy(&ast);
  token_array_destroy(&tokens);

  if (er_has_errors()) {
    er_print_all_errors();
  }

  er_free();

  return EXIT_SUCCESS;
}
