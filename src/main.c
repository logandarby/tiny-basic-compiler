// -------------------------------------
// TEENY TINY BASIC COMPILER
//
// Logan Darby
//
// A compiler for the TINY Basic Language
// specification, with some added features for good
// measure.
// -------------------------------------

#include "arg_parse.h"
#include "ast/ast_utils.h"
#include "backend/emitter-x86.h"
#include "common/error_reporter.h"
#include "common/file_reader.h"
#include "common/symbol_table.h"
#include "common/timer.h"
#include "compiler.h"
#include "dz_debug.h"
#include "frontend/lexer/lexer.h"
#include "frontend/parser/parser.h"
#include "platform.h"
#include <stb_ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Compiler Argument Config

static const FlagSpec FLAG_SPEC[] = {
    FLAG('c', "code", "Interpret the input_file as a code string literal"),
    FLAG_WITH_VALUE('t', "target", "Target to assemble to"),
    FLAG('h', "help", "Show this help message"),
    FLAG('v', "verbose", "Enable verbose output"),
    FLAG_WITH_VALUE('o', "output-file", "The name of the file to output to"),
};
static const ArgSpec ARG_SPEC[] = {REQUIRED_ARG(
    "input_file_or_literal", "The TINY BASIC file to assemble (or code literal "
                             "if compiling with the \"-c\" flag)")};
static const ParserSpec PARSER_SPEC =
    PARSER_SPEC("Teeny", "A TINY BASIC compiler", FLAG_SPEC, ARG_SPEC);

// CONSTANTS
static const char *SEP = "-------------------";
static const char *DEFAULT_OUT_FILE = "out.s";

// Helper Config Structs
typedef struct {
  bool verbose;
  const char *out_file;
} CompilerConfig;

// ---------------------
// Helper Functions
// ---------------------

CompilerConfig compiler_config_init(ParseResult *result) {
  const char *out_file = argparse_get_flag_value(result, "o");
  if (!out_file) {
    out_file = DEFAULT_OUT_FILE;
  }
  return (CompilerConfig){
      .verbose = argparse_has_flag(result, "v"),
      .out_file = out_file,
  };
}

void compiler_error(const char *restrict msg, ...) FORMAT_PRINTF(1, 2);
void compiler_error(const char *restrict msg, ...) {
  fprintf(stderr, "%s[ERROR]%s ", KRED, KNRM);
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr, "\n");
}

FileReader get_filereader_from_args(ParseResult *parse_result) {
  // Get the FileReader object depending on what the args have
  FileReader fr = NULL;
  const char *filename_or_code_literal =
      argparse_get_arg_value(parse_result, "input_file_or_literal");
  if (!argparse_has_flag(parse_result, "c") && filename_or_code_literal) {
    const char *filename = filename_or_code_literal;
    fr = filereader_init(filename);
    if (!fr) {
      compiler_error("File %s\"%s\"%s not found. Error: %s", KCYN, filename,
                     KNRM, strerror(errno));
      return NULL;
    }
  } else if (argparse_has_flag(parse_result, "c")) {
    const char *code = filename_or_code_literal;
    fr = filereader_init_from_string(code);
    if (!fr) {
      compiler_error("Invalid code literal provided.");
      return NULL;
    }
  }
  return fr;
}

// --------------
// MAIN LOOP
// --------------

int main(const int argc, const char **argv) {
  // Check host info
  if (HOST_INFO.os != OS_WINDOWS && HOST_INFO.os != OS_LINUX) {
    compiler_error("Target OS is not supported. Teeny may be used with either "
                   "%sLinux%s or %sWindows%s",
                   KCYN, KNRM, KCYN, KNRM);
    return EXIT_FAILURE;
  }
  if (HOST_INFO.arch != ARCH_X86_64) {
    compiler_error("Target architecture is not supported. Teeny can be used "
                   "only with 64-bit x86 architectures.");
    return EXIT_FAILURE;
  }

  // Parse Arguments
  ArgParser *argparser = argparse_create(&PARSER_SPEC);
  if (!argparser) {
    compiler_error("Could not parse arguments.");
    return EXIT_FAILURE;
  }
  ParseResult *parse_result = argparse_parse(argparser, argc, argv);
  if (!parse_result || !argparse_is_success(parse_result)) {
    compiler_error("Invalid arguments: %s", argparse_get_error(parse_result));
    argparse_print_help(argparser);
    argparse_free_parser(argparser);
    argparse_free_result(parse_result);
    return EXIT_FAILURE;
  }

  CompilerConfig config = compiler_config_init(parse_result);

  static int exit_code = EXIT_SUCCESS;

  // Start compiler timer
  Timer compiler_timer;
  timer_init(&compiler_timer);
  timer_start(&compiler_timer);

  // Read code (from file or literal)
  FileReader fr = get_filereader_from_args(parse_result);
  if (!fr) {
    argparse_print_help(argparser);
    argparse_free_parser(argparser);
    argparse_free_result(parse_result);
    return EXIT_FAILURE;
  }

  // Actual parsing logic
  TokenArray tokens = lexer_parse(fr);
  filereader_destroy(&fr);
  AST ast = ast_parse(tokens);
  if (config.verbose) {
    printf("%s AST PRINT %s\n", SEP, SEP);
    ast_print(&ast);
  }

  // Report errors
  if (er_has_errors()) {
    er_print_all_errors();
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }

  // Debug print symbol tables
  VariableTable *vars = variables_collect_from_ast(&ast);
  if (config.verbose) {
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
    emit_x86(&HOST_INFO, stdout, &ast, vars);
    printf("%s END DEBUG OUTPUT %s\n", SEP, SEP);
  }

  // Open file and emit asm
  FILE *out_file = fopen(config.out_file, "w");
  emit_x86(&HOST_INFO, out_file, &ast, vars);
  fclose(out_file);
  variables_destroy(vars);

  // Stop timer
  timer_stop(&compiler_timer);
  printf("Compiler finished in %.02f seconds\n",
         timer_elapsed_seconds(&compiler_timer));

cleanup:
  argparse_free_result(parse_result);
  argparse_free_parser(argparser);
  ast_destroy(&ast);
  token_array_destroy(&tokens);
  er_free();

  return exit_code;
}
