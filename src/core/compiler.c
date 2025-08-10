#include "compiler.h"
#include "config.h"

CompilerConfig compiler_config_init(ParseResult *result) {
  const char *out_file = argparse_get_flag_value(result, "o");
  if (!out_file) {
    out_file = DEFAULT_OUT_FILE;
  }
  PlatformInfo target = HOST_INFO;
  const char *target_triple = argparse_get_flag_value(result, "t");
  if (target_triple) {
    if (!parse_target_triple(target_triple, &target)) {
      target = HOST_INFO;
    }
  }
  return (CompilerConfig){
      .verbose = argparse_has_flag(result, "v"),
      .out_file = out_file,
      .target = target,
      .target_is_host = (memcmp(&target, &HOST_INFO, sizeof(target)) == 0),
  };
}
