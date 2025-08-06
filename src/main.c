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
#include "backend/emitter-x86.h"
#include "common/error_reporter.h"
#include "common/file_reader.h"
#include "common/symbol_table.h"
#include "common/timer.h"
#include "core/args.h"
#include "dz_debug.h"
#include "frontend/lexer/lexer.h"
#include "frontend/parser/parser.h"
#include "platform.h"
#include <stb_ds.h>
#include <stdio.h>
#include <stdlib.h>

static const bool VERBOSE = false;

int main(const int argc, const char **argv) {

  if (HOST_INFO.os != OS_WINDOWS && HOST_INFO.os != OS_LINUX) {
    fprintf(stderr,
            "%s[ERROR]%s Target OS is not supported. Teeny may be used with "
            "either %sLinux%s or %sWindows%s\n",
            KRED, KNRM, KCYN, KNRM, KCYN, KNRM);
    return EXIT_FAILURE;
  }
  if (HOST_INFO.arch != ARCH_X86_64) {
    fprintf(stderr,
            "%s[ERROR]%s Target architecture is not supported. Teeny can be "
            "used only with 64 bit x86 architecture\n",
            KRED, KNRM);
    return EXIT_FAILURE;
  }

  static int exit_code = EXIT_SUCCESS;
  static const char *SEP = "-------------------";
  Args args = parse_args(argc, argv);

  Timer compiler_timer;
  timer_init(&compiler_timer);
  timer_start(&compiler_timer);

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
  if (VERBOSE) {
    printf("%s AST PRINT %s\n", SEP, SEP);
    ast_print(&ast);
  }

  if (er_has_errors()) {
    er_print_all_errors();
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }

  // Debug print symbol tables

  VariableTable *vars = variables_collect_from_ast(&ast);
  if (VERBOSE) {
    printf("%s SYMBOL TABLE %s\n", SEP, SEP);
    for (size_t i = 0; i < shlenu(vars->symbol_table); i++) {
      SymbolHash sym = vars->symbol_table[i];
      printf("Key: %s,\tValue: %" PRIu32 "\n", sym.key, sym.value.label);
    }
    printf("%s LITERAL TABLE %s\n", SEP, SEP);
    for (size_t i = 0; i < shlenu(vars->literal_table); i++) {
      LiteralHash lit = vars->literal_table[i];
      printf("Key: %s,\tValue: %" PRIu32 "\n", lit.key, lit.value.label);
    }

    // Debug print generated ASM
    printf("%s EMITTED ASM %s\n", SEP, SEP);
    emit_x86(HOST_INFO, stdout, &ast, vars);
    printf("%s END DEBUG OUTPUT %s\n", SEP, SEP);
  }

  // Open file and emit asm
  FILE *out_file = fopen("out.s", "w");
  emit_x86(HOST_INFO, out_file, &ast, vars);
  fclose(out_file);
  variables_destroy(vars);

  timer_stop(&compiler_timer);
  printf("Compiler finished in %.02f seconds\n",
         timer_elapsed_seconds(&compiler_timer));

cleanup:
  ast_destroy(&ast);
  token_array_destroy(&tokens);
  er_free();

  return exit_code;
}
