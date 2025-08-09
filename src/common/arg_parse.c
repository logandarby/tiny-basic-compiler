#include "arg_parse.h"

typedef struct {
  char *short_name;
  char *long_name;
  char *description;
  bool requires_value;
  bool is_required;
} FlagDef;

typedef struct {
  char *name;
  char *description;
  bool is_required;
} ArgDef;

typedef struct {
  const FlagDef *def;
  char *value;
  bool is_present;
} ParsedFlag;

typedef struct {
  const ArgDef *def;
  char *value;
} ParsedArg;

typedef struct ArgParserHandle {
  FlagDef *flags;
  int flag_count;

  ArgDef *args;
  int arg_count;

  char *program_name;
  char *description;
} ArgParserHandle;

typedef struct ParseResultHandle {
  ParsedFlag *flags;
  int flag_count;

  ParsedArg *args;
  int arg_count;

  char **remaining_args;
  int remaining_count;

  bool success;
  char *error_message;
} ParseResultHandle;

// === Helper Functions ===

static char *safe_strdup(const char *str) { return str ? strdup(str) : NULL; }

static const FlagDef *find_flag(const ArgParserHandle *parser, const char *name,
                                bool is_long) {
  for (int i = 0; i < parser->flag_count; i++) {
    const FlagDef *flag = &parser->flags[i];
    if (is_long && flag->long_name && strcmp(flag->long_name, name) == 0) {
      return flag;
    }
    if (!is_long && flag->short_name && strcmp(flag->short_name, name) == 0) {
      return flag;
    }
  }
  return NULL;
}

// === Main API Implementation ===

ArgParserHandle *argparse_create(const ParserSpec *spec) {
  if (!spec)
    return NULL;

  ArgParserHandle *parser = calloc(1, sizeof(ArgParserHandle));
  if (!parser)
    return NULL;

  // Copy basic info
  parser->program_name = safe_strdup(spec->program_name);
  parser->description = safe_strdup(spec->description);

  // Copy flags
  if (spec->flags && spec->flag_count > 0) {
    parser->flags = calloc(spec->flag_count, sizeof(FlagDef));
    if (!parser->flags) {
      argparse_free_parser(parser);
      return NULL;
    }
    parser->flag_count = spec->flag_count;

    for (int i = 0; i < spec->flag_count; i++) {
      parser->flags[i].short_name = safe_strdup(spec->flags[i].short_name);
      parser->flags[i].long_name = safe_strdup(spec->flags[i].long_name);
      parser->flags[i].description = safe_strdup(spec->flags[i].description);
      parser->flags[i].requires_value = spec->flags[i].requires_value;
      parser->flags[i].is_required = spec->flags[i].is_required;
    }
  }

  // Copy args
  if (spec->args && spec->arg_count > 0) {
    parser->args = calloc(spec->arg_count, sizeof(ArgDef));
    if (!parser->args) {
      argparse_free_parser(parser);
      return NULL;
    }
    parser->arg_count = spec->arg_count;

    for (int i = 0; i < spec->arg_count; i++) {
      parser->args[i].name = safe_strdup(spec->args[i].name);
      parser->args[i].description = safe_strdup(spec->args[i].description);
      parser->args[i].is_required = spec->args[i].is_required;
    }
  }

  return parser;
}

ParseResultHandle *argparse_parse(ArgParserHandle *parser, int argc,
                                  const char **argv) {
  if (!parser)
    return NULL;

  ParseResultHandle *result = calloc(1, sizeof(ParseResultHandle));
  if (!result)
    return NULL;

  // Initialize flag results
  if (parser->flag_count > 0) {
    result->flags = calloc(parser->flag_count, sizeof(ParsedFlag));
    if (!result->flags) {
      argparse_free_result(result);
      return NULL;
    }
    result->flag_count = parser->flag_count;

    for (int i = 0; i < parser->flag_count; i++) {
      result->flags[i].def = &parser->flags[i];
      result->flags[i].is_present = false;
      result->flags[i].value = NULL;
    }
  }

  // Initialize arg results
  if (parser->arg_count > 0) {
    result->args = calloc(parser->arg_count, sizeof(ParsedArg));
    if (!result->args) {
      argparse_free_result(result);
      return NULL;
    }
    result->arg_count = parser->arg_count;

    for (int i = 0; i < parser->arg_count; i++) {
      result->args[i].def = &parser->args[i];
      result->args[i].value = NULL;
    }
  }

  int positional_index = 0;

  // Parse command line
  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];

    if (arg[0] == '-' && arg[1] != '\0') {
      // It's a flag
      const FlagDef *flag_def = NULL;
      bool is_long = (arg[1] == '-');
      const char *flag_name = is_long ? arg + 2 : arg + 1;

      // Handle empty long flag (just --)
      if (is_long && flag_name[0] == '\0') {
        // End of options marker
        i++;
        // Add remaining args to positional or remaining
        for (; i < argc; i++) {
          if (positional_index < parser->arg_count) {
            result->args[positional_index].value = strdup(argv[i]);
            positional_index++;
          } else {
            result->remaining_args =
                realloc(result->remaining_args,
                        (result->remaining_count + 1) * sizeof(char *));
            result->remaining_args[result->remaining_count++] = strdup(argv[i]);
          }
        }
        break;
      }

      flag_def = find_flag(parser, flag_name, is_long);

      if (!flag_def) {
        result->success = false;
        asprintf(&result->error_message, "Unknown flag: %s", arg);
        return result;
      }

      // Find the corresponding parsed flag
      ParsedFlag *parsed_flag = NULL;
      for (int j = 0; j < result->flag_count; j++) {
        if (result->flags[j].def == flag_def) {
          parsed_flag = &result->flags[j];
          break;
        }
      }

      if (!parsed_flag) {
        result->success = false;
        asprintf(&result->error_message, "Internal error: flag not found");
        return result;
      }

      parsed_flag->is_present = true;

      if (flag_def->requires_value) {
        if (i + 1 >= argc || argv[i + 1][0] == '-') {
          result->success = false;
          asprintf(&result->error_message, "Flag %s requires a value", arg);
          return result;
        }
        parsed_flag->value = strdup(argv[++i]);
      }
    } else {
      // Positional argument
      if (positional_index < parser->arg_count) {
        result->args[positional_index].value = strdup(arg);
        positional_index++;
      } else {
        // Extra arguments - add to remaining
        result->remaining_args =
            realloc(result->remaining_args,
                    (result->remaining_count + 1) * sizeof(char *));
        if (!result->remaining_args) {
          result->success = false;
          asprintf(&result->error_message, "Out of memory");
          return result;
        }
        result->remaining_args[result->remaining_count++] = strdup(arg);
      }
    }
  }

  // Check for required flags
  for (int i = 0; i < result->flag_count; i++) {
    if (result->flags[i].def->is_required && !result->flags[i].is_present) {
      result->success = false;
      const char *name = result->flags[i].def->long_name
                             ? result->flags[i].def->long_name
                             : result->flags[i].def->short_name;
      asprintf(&result->error_message, "Required flag missing: %s%s",
               result->flags[i].def->long_name ? "--" : "-", name);
      return result;
    }
  }

  // Check for required arguments
  for (int i = 0; i < result->arg_count; i++) {
    if (result->args[i].def->is_required && !result->args[i].value) {
      result->success = false;
      asprintf(&result->error_message, "Required argument missing: %s",
               result->args[i].def->name);
      return result;
    }
  }

  result->success = true;
  return result;
}

// === Query Functions ===

bool argparse_has_flag(const ParseResultHandle *result, const char *flag_name) {
  if (!result || !flag_name)
    return false;

  for (int i = 0; i < result->flag_count; i++) {
    const FlagDef *def = result->flags[i].def;
    if ((def->short_name && strcmp(def->short_name, flag_name) == 0) ||
        (def->long_name && strcmp(def->long_name, flag_name) == 0)) {
      return result->flags[i].is_present;
    }
  }
  return false;
}

const char *argparse_get_flag_value(const ParseResultHandle *result,
                                    const char *flag_name) {
  if (!result || !flag_name)
    return NULL;

  for (int i = 0; i < result->flag_count; i++) {
    const FlagDef *def = result->flags[i].def;
    if ((def->short_name && strcmp(def->short_name, flag_name) == 0) ||
        (def->long_name && strcmp(def->long_name, flag_name) == 0)) {
      return result->flags[i].value;
    }
  }
  return NULL;
}

const char *argparse_get_arg_value(const ParseResultHandle *result,
                                   const char *arg_name) {
  if (!result || !arg_name)
    return NULL;

  for (int i = 0; i < result->arg_count; i++) {
    if (result->args[i].def->name &&
        strcmp(result->args[i].def->name, arg_name) == 0) {
      return result->args[i].value;
    }
  }
  return NULL;
}

char **argparse_get_remaining_args(const ParseResultHandle *result,
                                   int *count) {
  if (!result || !count) {
    if (count)
      *count = 0;
    return NULL;
  }

  *count = result->remaining_count;
  return result->remaining_args;
}

bool argparse_is_success(const ParseResultHandle *result) {
  return result ? result->success : false;
}

const char *argparse_get_error(const ParseResultHandle *result) {
  return (result && !result->success) ? result->error_message : NULL;
}

// === Help Printing ===

void argparse_print_help(const ArgParserHandle *parser) {
  if (!parser)
    return;

  // Print usage line
  printf("Usage: %s", parser->program_name ? parser->program_name : "program");

  if (parser->flag_count > 0) {
    printf(" [OPTIONS]");
  }

  for (int i = 0; i < parser->arg_count; i++) {
    if (parser->args[i].is_required) {
      printf(" <%s>", parser->args[i].name);
    } else {
      printf(" [%s]", parser->args[i].name);
    }
  }
  printf("\n\n");

  // Print description
  if (parser->description) {
    printf("%s\n\n", parser->description);
  }

  // Print options
  if (parser->flag_count > 0) {
    printf("Options:\n");
    for (int i = 0; i < parser->flag_count; i++) {
      const FlagDef *flag = &parser->flags[i];
      printf("  ");

      if (flag->short_name) {
        printf("-%s", flag->short_name);
        if (flag->long_name)
          printf(", ");
      }
      if (flag->long_name) {
        printf("--%s", flag->long_name);
      }

      if (flag->requires_value) {
        printf(" <value>");
      }

      if (flag->description) {
        printf("    %s", flag->description);
      }

      if (flag->is_required) {
        printf(" (required)");
      }

      printf("\n");
    }
    printf("\n");
  }

  // Print arguments
  if (parser->arg_count > 0) {
    printf("Arguments:\n");
    for (int i = 0; i < parser->arg_count; i++) {
      const ArgDef *arg = &parser->args[i];
      printf("  %-15s %s", arg->name, arg->description ? arg->description : "");
      if (arg->is_required) {
        printf(" (required)");
      }
      printf("\n");
    }
  }
  printf("\n");
}

// === Cleanup Functions ===

void argparse_free_parser(ArgParserHandle *parser) {
  if (!parser)
    return;

  // Free flags
  if (parser->flags) {
    for (int i = 0; i < parser->flag_count; i++) {
      free(parser->flags[i].short_name);
      free(parser->flags[i].long_name);
      free(parser->flags[i].description);
    }
    free(parser->flags);
  }

  // Free args
  if (parser->args) {
    for (int i = 0; i < parser->arg_count; i++) {
      free(parser->args[i].name);
      free(parser->args[i].description);
    }
    free(parser->args);
  }

  free(parser->program_name);
  free(parser->description);
  free(parser);
}

void argparse_free_result(ParseResultHandle *result) {
  if (!result)
    return;

  // Free flag values
  if (result->flags) {
    for (int i = 0; i < result->flag_count; i++) {
      free(result->flags[i].value);
    }
    free(result->flags);
  }

  // Free arg values
  if (result->args) {
    for (int i = 0; i < result->arg_count; i++) {
      free(result->args[i].value);
    }
    free(result->args);
  }

  // Free remaining args
  if (result->remaining_args) {
    for (int i = 0; i < result->remaining_count; i++) {
      free(result->remaining_args[i]);
    }
    free(result->remaining_args);
  }

  free(result->error_message);
  free(result);
}
