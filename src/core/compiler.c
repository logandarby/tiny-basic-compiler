#include "compiler.h"
#include "../ast/ast_utils.h"
#include "../backend/assembly.h"
#include "../backend/emitter-x86.h"
#include "../common/error_reporter.h"
#include "../common/file_reader.h"
#include "../common/name_table.h"
#include "../common/timer.h"
#include "../core/core.h"
#include "../core/system.h"
#include "../frontend/lexer/lexer.h"
#include "../parser/parser.h"
#include "arg_parse.h"
#include "config.h"
#include "platform.h"
#include <stdlib.h>
#include <string.h>

CompilerConfig compiler_config_init(ParseResult *result) {
  // code/filename
  const char *arg_filename_or_code_literal =
      argparse_get_arg_value(result, "input_file_or_literal");
  char *filename_or_code_literal = NULL;
  if (arg_filename_or_code_literal)
    filename_or_code_literal = strdup(arg_filename_or_code_literal);

  // outfile
  const char *arg_out_file = argparse_get_flag_value(result, "o");
  char *out_file = NULL;
  if (!arg_out_file) {
    out_file = strdup(DEFAULT_OUT_FILE);
  } else {
    out_file = strdup(arg_out_file);
  }

  // triple & target
  PlatformInfo target = HOST_INFO;
  const char *arg_target_triple = argparse_get_flag_value(result, "t");
  char *triple = NULL;
  if (arg_target_triple) {
    if (!parse_target_triple(arg_target_triple, &target)) {
      triple = NULL;
    } else {
      triple = strdup(arg_target_triple);
    }
  } else {
    triple = platform_info_to_triple(&HOST_INFO);
  }
  return (CompilerConfig){
      .verbose = argparse_has_flag(result, "v"),
      .out_file = out_file,
      .target = target,
      .target_is_host = (memcmp(&target, &HOST_INFO, sizeof(target)) == 0),
      .triple = triple,
      .filename_or_code_literal = filename_or_code_literal,
      .is_code_literal = argparse_has_flag(result, "c"),
      .emit_format = argparse_has_flag(result, "emit-asm") ? EMIT_X86_ASSEMBLY
                                                           : EMIT_EXECUTABLE};
}

void compiler_config_free(CompilerConfig *config) {
  if (!config)
    return;
  if (config->out_file)
    free(config->out_file);
  if (config->triple)
    free(config->triple);
  if (config->filename_or_code_literal)
    free(config->filename_or_code_literal);
}

// Helper functions

void compiler_error(const char *restrict msg, ...) {
  fprintf(stderr, "%s[ERROR]%s ", KRED, KNRM);
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr, "\n");
}

FileReader get_filereader_from_config(const CompilerConfig *config) {
  // Get the FileReader object depending on what the args have
  FileReader fr = NULL;
  if (!config->is_code_literal && config->filename_or_code_literal) {
    const char *filename = config->filename_or_code_literal;
    fr = filereader_init(filename);
    if (!fr) {
      compiler_error("File %s\"%s\"%s not found. Error: %s", KCYN, filename,
                     KNRM, strerror(errno));
      return NULL;
    }
  } else if (config->is_code_literal && config->filename_or_code_literal) {
    const char *code = config->filename_or_code_literal;
    fr = filereader_init_from_string(code);
    if (!fr) {
      compiler_error("Invalid code literal provided.");
      return NULL;
    }
  }
  return fr;
}

bool compiler_execute(const CompilerConfig *config) {
  // Check if toolchain is available
  if (system("gcc --version > /dev/null 2>&1") != EXIT_SUCCESS) {
    if (HOST_INFO.os == OS_WINDOWS) {
      compiler_error(
          "GCC is not available on your system. Please install MinGW-64");
    } else {
      compiler_error("GCC is not available on your system. Please install GCC");
    }
    return false;
  }

  // exit_code used in cleanup label
  static bool exit_code = true;

  // Error if anything in the config is unknown
  if (config->target.os == OS_UNKNOWN || config->target.arch == ARCH_UNKNOWN ||
      config->target.abi == ABI_UNKNOWN) {
    if (config->triple) {
      compiler_error(
          "Invalid target triple %s. The target triple should take the form "
          "\"arch-os\", where arch and os are supported. Below are the "
          "currently supported targets",
          config->triple);
      print_supported_platforms("\t -");
    }
    if (config->target.os == OS_UNKNOWN) {
      compiler_error("Unknown target OS. Aborting.");
    } else if (config->target.arch == ARCH_UNKNOWN) {
      compiler_error("Unknown target architecture. Aborting.");
    } else if (config->target.abi == ABI_UNKNOWN) {
      compiler_error("Unknown target ABI. Aborting.");
    }
    return false;
  }
  // Error if target is not supported
  if (!is_supported_os(&config->target)) {
    compiler_error("Target OS is not supported. Teeny may be used with the "
                   "following targets:");
    print_supported_platforms("\t -");
    return false;
  }
  if (!is_supported_arch(&config->target)) {
    compiler_error("Target architecture is not supported. Teeny can be used "
                   "only with 64-bit x86 architectures. The following targets "
                   "are supported:");
    print_supported_platforms("\t -");
    return false;
  }

  // Start compiler timer
  Timer compiler_timer;
  timer_init(&compiler_timer);
  timer_start(&compiler_timer);

  // Read code (from file or literal)
  FileReader fr = get_filereader_from_config(config);
  if (!fr) {
    return false;
  }

  // Print verbose target
  if (config->verbose) {
    printf("Compiling to target %s\n", config->triple);
  }

  // Actual parsing logic
  TokenArray tokens = lexer_parse(fr);
  filereader_destroy(&fr);
  AST ast = ast_parse(tokens);
  if (config->verbose) {
    printf("%s AST PRINT %s\n", SEP, SEP);
    ast_print(&ast);
  }

  // Report errors
  if (er_has_errors()) {
    er_print_all_errors();
    exit_code = false;
    goto cleanup;
  }

  // Debug print symbol tables
  NameTable *vars = name_table_collect_from_ast(&ast);
  if (config->verbose) {
    printf("%s SYMBOL TABLE %s\n", SEP, SEP);
    for (size_t i = 0; i < shlenu(vars->symbol_table); i++) {
      SymbolHash sym = vars->symbol_table[i];
      printf("Key: %s,\tPos: %" PRIu32 ":%" PRIu32 "\n", sym.key,
             sym.value.file_pos.line, sym.value.file_pos.col);
    }
    printf("%s LABEL TABLE %s\n", SEP, SEP);
    for (size_t i = 0; i < shlenu(vars->label_table); i++) {
      LabelHash label = vars->label_table[i];
      printf("Label: %s,\tPos: %" PRIu32 ":%" PRIu32 "\n", label.key,
             label.value.file_pos.line, label.value.file_pos.col);
    }
    printf("%s LITERAL TABLE %s\n", SEP, SEP);
    for (size_t i = 0; i < shlenu(vars->literal_table); i++) {
      LiteralHash lit = vars->literal_table[i];
      printf("Key: %s,\tValue: %" PRIu32 "\n", lit.key, lit.value.label);
    }

    // Debug print generated ASM
    printf("%s EMITTED ASM %s\n", SEP, SEP);
    emit_x86(&config->target, stdout, &ast, vars);
    printf("%s END DEBUG OUTPUT %s\n", SEP, SEP);
  }

  char tmp_asm_file[PATH_MAX];
  // Open file and emit asm
  // IF the emit format is exec, create a temp file for the asm
  if (config->emit_format == EMIT_EXECUTABLE) {
    FILE *asm_file = create_named_tmpfile(tmp_asm_file, sizeof(tmp_asm_file));
    if (!asm_file) {
      compiler_error("SYSTEM ERROR: Could not create temporary file: %s",
                     strerror(errno));
      exit_code = false;
      goto cleanup;
    }
    emit_x86(&config->target, asm_file, &ast, vars);
    fclose(asm_file);
    asm_file = NULL;
  } else if (config->emit_format == EMIT_X86_ASSEMBLY) {
    // IF the emit format is assembly, then open the out_file to emit to
    FILE *asm_file = fopen(config->out_file, "w");
    if (!asm_file) {
      compiler_error(
          "SYSTEM ERROR: Could not open output file %s for writing: %s",
          config->out_file, strerror(errno));
      exit_code = false;
      goto cleanup;
    }
    emit_x86(&config->target, asm_file, &ast, vars);
    fclose(asm_file);
    asm_file = NULL;
    strncpy(tmp_asm_file, config->out_file, sizeof(tmp_asm_file) - 1);
  }
  name_table_destroy(vars);

  // Stop timer
  timer_stop(&compiler_timer);
  printf("Compiler finished in %.02f seconds\n",
         timer_elapsed_seconds(&compiler_timer));

  // ======= STAGE 2: Assembly ==========
  // Only emitted if emit_format == EMIT_EXECUTABLE flag isn't specified
  if (config->emit_format == EMIT_X86_ASSEMBLY) {
    goto cleanup;
  }

  Timer asssembler_timer;
  timer_init(&asssembler_timer);
  timer_start(&asssembler_timer);

  // Invoke GCC on file
  AssemblerInfo cmd;
  if (!assembler_init(&cmd, config)) {
    compiler_error("Target is not supported");
    exit_code = false;
    goto cleanup;
  }
  if (!assembler_is_available(&cmd)) {
    compiler_error("Assmebler is not installed on the system");
    assembler_print_help(&cmd);
    exit_code = false;
    goto cleanup;
  }
  if (!assembler_invoke(&cmd, tmp_asm_file, config->out_file)) {
    compiler_error("Assembly failed");
    exit_code = false;
    goto cleanup;
  }

  // Stop assembler timer
  timer_stop(&asssembler_timer);
  printf("Assembler finished in %.02f seconds\n",
         timer_elapsed_seconds(&asssembler_timer));

cleanup:
  ast_destroy(&ast);
  token_array_destroy(&tokens);
  er_free();
  return exit_code;
}
