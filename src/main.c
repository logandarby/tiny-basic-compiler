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
#include "backend/assembly.h"
#include "backend/emitter-x86.h"
#include "common/error_reporter.h"
#include "common/file_reader.h"
#include "common/symbol_table.h"
#include "common/timer.h"
#include "compiler.h"
#include "core/config.h"
#include "core/system.h"
#include "dz_debug.h"
#include "frontend/lexer/lexer.h"
#include "frontend/parser/parser.h"
#include <stb_ds.h>
#include <stdlib.h>
#include <string.h>

// Helper functions

void compiler_error(const char *restrict msg, ...) FORMAT_PRINTF(1, 2);
void compiler_error(const char *restrict msg, ...) {
  fprintf(stderr, "%s[ERROR]%s ", KRED, KNRM);
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr, "\n");
}

bool is_in_array(const int array[], const size_t count, const int elem) {
  for (size_t i = 0; i < count; i++) {
    if (array[i] == elem)
      return true;
  }
  return false;
}

void parse_debug_commands_and_exit(const ArgParser *parser,
                                   ParseResult *result) {
  if (argparse_has_flag(result, "h")) {
    argparse_print_help(parser);
    exit(EXIT_SUCCESS);
  } else if (argparse_has_flag(result, "host-info")) {
    char *triple = platform_info_to_triple(&HOST_INFO);
    printf("%s\n", triple);
    free(triple);
    exit(EXIT_SUCCESS);
  }
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
  // Check if toolchain is available
  if (system("gcc --version > /dev/null 2>&1") != EXIT_SUCCESS) {
    if (HOST_INFO.os == OS_WINDOWS) {
      compiler_error(
          "GCC is not available on your system. Please install MinGW-64");
    } else {
      compiler_error("GCC is not available on your system. Please install GCC");
    }
    return EXIT_FAILURE;
  }

  static int exit_code = EXIT_SUCCESS;

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
    exit_code = EXIT_FAILURE;
    goto arg_cleanup;
  }

  CompilerConfig config = compiler_config_init(parse_result);

  // Error if anything in the config is unknown
  if (config.target.os == OS_UNKNOWN || config.target.arch == ARCH_UNKNOWN ||
      config.target.abi == ABI_UNKNOWN) {
    const char *triple = argparse_get_flag_value(parse_result, "t");
    if (triple) {
      compiler_error(
          "Invalid target triple %s. The target triple should take the form "
          "\"arch-os\", where arch and os are supported. Example: "
          "x86_64-windows",
          triple);
    }
    if (config.target.os == OS_UNKNOWN) {
      compiler_error("Unknown target OS. Aborting.");
    } else if (config.target.arch == ARCH_UNKNOWN) {
      compiler_error("Unknown target architecture. Aborting.");
    } else if (config.target.abi == ABI_UNKNOWN) {
      compiler_error("Unknown target ABI. Aborting.");
    }
    exit_code = EXIT_FAILURE;
    goto arg_cleanup;
  }
  // Error if target is not supported
  if (!is_in_array((const int *)SUPPORTED_OS, array_size(SUPPORTED_OS),
                   config.target.os)) {
    compiler_error("Target OS is not supported. Teeny may be used with either "
                   "%sLinux%s or %sWindows%s",
                   KCYN, KNRM, KCYN, KNRM);
    exit_code = EXIT_FAILURE;
    goto arg_cleanup;
  }
  if (!is_in_array((const int *)SUPPORTED_ARCH, array_size(SUPPORTED_ARCH),
                   config.target.arch)) {
    compiler_error("Target architecture is not supported. Teeny can be used "
                   "only with 64-bit x86 architectures.");
    exit_code = EXIT_FAILURE;
    goto arg_cleanup;
  }

  // Parse any commands like --help and exit if they exist
  parse_debug_commands_and_exit(argparser, parse_result);

  // Start compiler timer
  Timer compiler_timer;
  timer_init(&compiler_timer);
  timer_start(&compiler_timer);

  // Read code (from file or literal)
  FileReader fr = get_filereader_from_args(parse_result);
  if (!fr) {
    argparse_print_help(argparser);
    exit_code = EXIT_FAILURE;
    goto arg_cleanup;
  }

  // Print verbose target
  if (config.verbose) {
    char *triple = platform_info_to_triple(&config.target);
    printf("Compiling to target %s\n", triple);
    free(triple);
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
    emit_x86(&config.target, stdout, &ast, vars);
    printf("%s END DEBUG OUTPUT %s\n", SEP, SEP);
  }

  // Open file and emit asm
  char tmp_asm_file[PATH_MAX];
  FILE *asm_file = create_named_tmpfile(tmp_asm_file, sizeof(tmp_asm_file));
  if (!asm_file) {
    compiler_error("SYSTEM ERROR: Could not create temporary file: %s",
                   strerror(errno));
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }
  FILE *out_file = fopen(config.out_file, "w");
  if (!out_file) {
    compiler_error("SYSTEM ERROR: Could not create output file: %s",
                   strerror(errno));
  }
  emit_x86(&config.target, asm_file, &ast, vars);
  fclose(asm_file);
  asm_file = NULL;

  // Stop timer
  timer_stop(&compiler_timer);
  printf("Compiler finished in %.02f seconds\n",
         timer_elapsed_seconds(&compiler_timer));

  // ======= STAGE 2: Assembly ==========

  Timer asssembler_timer;
  timer_init(&asssembler_timer);
  timer_start(&asssembler_timer);

  // Invoke GCC on file
  AssemblerInfo cmd;
  if (!assembler_init(&cmd, &config)) {
    compiler_error("Target is not supported");
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }
  if (!assembler_is_available(&cmd)) {
    compiler_error("Assmebler is not installed on the system");
    assembler_print_help(&cmd);
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }
  if (!assembler_invoke(&cmd, tmp_asm_file, config.out_file)) {
    compiler_error("SYSTEM ERROR: Could not execute assembler. Errno %s",
                   strerror(errno));
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }

  // Stop assembler timer
  timer_stop(&asssembler_timer);
  printf("Assembler finished in %.02f seconds\n",
         timer_elapsed_seconds(&asssembler_timer));

  fclose(out_file);
  out_file = NULL;
  variables_destroy(vars);

cleanup:
  ast_destroy(&ast);
  token_array_destroy(&tokens);
  er_free();
arg_cleanup:
  argparse_free_result(parse_result);
  argparse_free_parser(argparser);
  return exit_code;
}
