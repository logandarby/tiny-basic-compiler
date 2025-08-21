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
#include "compiler.h"
#include "config.h"
#include "core.h"
#include "dz_debug.h"
#include "platform.h"
#include <stdlib.h>

void parse_debug_commands_and_exit(const ArgParser *parser,
                                   ParseResult *result) {
  if (argparse_has_flag(result, "h")) {
    argparse_print_help(parser);
    exit(EXIT_SUCCESS);
  }

  if (argparse_has_flag(result, "host-info")) {
    char *triple = platform_info_to_triple(&HOST_INFO);
    printf("%s\n", triple);
    free(triple);
    exit(EXIT_SUCCESS);
  }

  if (argparse_has_flag(result, "list-targets")) {
    printf("Supported targets:\n");
    print_supported_platforms("\t -");
    exit(EXIT_SUCCESS);
  }
}

// --------------
// MAIN LOOP
// --------------

int main(const int argc, const char **argv) {

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
    if (parse_result) {
      argparse_free_result(parse_result);
    }
    return EXIT_FAILURE;
  }

  // Parse any commands like --help and exit if they exist
  parse_debug_commands_and_exit(argparser, parse_result);

  if (!argparse_get_arg_value(parse_result, "input_file_or_literal")) {
    compiler_error("Please input a file or code literal (using the -c flag)");
    argparse_free_parser(argparser);
    argparse_free_result(parse_result);
    return EXIT_FAILURE;
  }

  CompilerConfig config = compiler_config_init(parse_result);
  const bool success = compiler_execute(&config);
  compiler_config_free(&config);
  argparse_free_result(parse_result);
  argparse_free_parser(argparser);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
