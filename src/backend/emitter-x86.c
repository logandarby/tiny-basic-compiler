#include "emitter-x86.h"
#include "ast.h"
#include "batched_writer.h"
#include "compiler_compatibility.h"
#include "dz_debug.h"
#include "name_table.h"
#include "platform.h"
#include "token.h"
#include <stb_ds.h>
#include <stdarg.h>
#include <stdio.h>

const char *PREAMBLE = ".intel_syntax noprefix\n"
                       ".data\n"
                       "\tprint_integer_fmt: .string \"%d\\n\"\n"
                       "\tprint_string_fmt: .string \"%s\\n\"\n"
                       "\tinput_string_fmt: .string \"%100s\"\n";
const char *MAIN_PREAMBLE = ".text\n"
                            "\t.global main\n"
                            "main:\n";
const char *LINUX_POSTAMBLE = ".section .note.GNU-stack,\"\",@progbits\n";

// Function names

#define PRINT_INTEGER "print_integer"
#define PRINT_STRING "print_string"
#define INPUT_INTEGER "input_integer"

#define PRINT_INTEGER_FMT_STR "print_integer_fmt"
#define PRINT_STRING_FMT_STR "print_string_fmt"
#define INPUT_INTEGER_FMT_STR "input_string_fmt"

// Internal delimiters

#define LITERAL_DELIMITER "_static_"
#define SYMBOL_DELIMITER "_var_"
#define LABEL_DELIMITER ".LAB"
#define INTERNAL_LABEL_DELIMITER ".ILAB"

// ----------------------
// Emitter Internals
// ----------------------

typedef struct {
  BatchedWriter writer;
  const PlatformInfo *platform_info;
  const CallingConvention *cc;
  uint32_t control_flow_label; // Used to create unique labels for IF and WHILE
                               // statement
  NameTable *table;            // Non-owning reference
  AST *ast;                    // Non-owning reference
} Emitter;

Emitter emitter_init(const PlatformInfo *platform_info, FILE *file, AST *ast,
                     NameTable *table) {
  return (Emitter){
      .ast = ast,
      .writer = batched_writer_init(file),
      .table = table,
      .control_flow_label = 0,
      .platform_info = platform_info,
      .cc = get_calling_convention(platform_info),
  };
}

uint32_t emitter_get_label(Emitter *emit) { return emit->control_flow_label++; }

void _emit_literals(Emitter *emit) {
  const LiteralTable literals = emit->table->literal_table;
  const uint32_t literal_len = shlenu(literals);
  for (uint32_t i = 0; i < literal_len; i++) {
    const LiteralHash lit = literals[i];
    batched_writer_printf(&emit->writer, "\t%s%" PRIu32 ": .string \"%s\"\n",
                          LITERAL_DELIMITER, lit.value.label, lit.key);
  }
}

// Generic emitter for complex cases
void _emit_instr(Emitter *emit, const char *msg, ...) FORMAT_PRINTF(2, 3);
void _emit_instr(Emitter *emit, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  batched_writer_write(&emit->writer, "\t");
  batched_writer_vprintf(&emit->writer, msg, args);
  batched_writer_write(&emit->writer, "\n");
  va_end(args);
}

void _emit_label(Emitter *emit, const char *msg) {
  batched_writer_printf(&emit->writer, "%s:\n", msg);
}

void _emit_mov(Emitter *emit, const char *dest, const char *src) {
  _emit_instr(emit, "mov %s, %s", dest, src);
}

void _emit_lea(Emitter *emit, const char *dest, const char *src) {
  _emit_instr(emit, "lea %s, %s", dest, src);
}

void _emit_lea_fmt(Emitter *emit, const char *dest, const char *src_fmt, ...)
    FORMAT_PRINTF(3, 4);
void _emit_lea_fmt(Emitter *emit, const char *dest, const char *src_fmt, ...) {
  va_list args;
  va_start(args, src_fmt);
  char src_buffer[256];
  vsnprintf(src_buffer, sizeof(src_buffer), src_fmt, args);
  va_end(args);
  _emit_instr(emit, "lea %s, %s", dest, src_buffer);
}

void _emit_push(Emitter *emit, const char *reg) {
  _emit_instr(emit, "push %s", reg);
}

void _emit_pop(Emitter *emit, const char *reg) {
  _emit_instr(emit, "pop %s", reg);
}

void _emit_sub(Emitter *emit, const char *a1, const char *a2) {
  _emit_instr(emit, "sub %s, %s", a1, a2);
}

void _emit_add(Emitter *emit, const char *a1, const char *a2) {
  _emit_instr(emit, "add %s, %s", a1, a2);
}

void _emit_func_preamble(Emitter *emit) {
  const CallingConvention *cc = emit->cc;
  _emit_push(emit, cc->rbp);
  _emit_mov(emit, cc->rbp, cc->rsp);
  if (cc->shadow_space) {
    _emit_instr(emit, "sub %s, %" PRIu8, cc->rsp, cc->shadow_space);
  }
}

void _emit_func_ret(Emitter *emit) {
  const CallingConvention *cc = emit->cc;
  if (cc->shadow_space) {
    _emit_instr(emit, "add %s, %" PRIu8, cc->rsp, cc->shadow_space);
  }
  _emit_instr(emit, "leave");
  _emit_instr(emit, "ret");
}

void _emit_syscall(Emitter *emit, const char *sys_call) {
  _emit_instr(emit, "xor %s, %s", emit->cc->ret_r, emit->cc->ret_r);
  _emit_instr(emit, "call %s", sys_call);
}

// Emit Helpers

// Helper that prints an integer
void _emit_print_int(Emitter *emit) {
  const CallingConvention *cc = emit->cc;
  _emit_label(emit, PRINT_INTEGER);
  _emit_func_preamble(emit);
  _emit_mov(emit, cc->arg_r[1], cc->arg_r[0]);
  _emit_instr(emit, "lea %s, " PRINT_INTEGER_FMT_STR "[%s]", cc->arg_r[0],
              cc->rip);
  _emit_syscall(emit, "printf");
  _emit_func_ret(emit);
}

// Helper that prints a string literal
void _emit_print_string(Emitter *emit) {
  const CallingConvention *cc = emit->cc;
  _emit_label(emit, PRINT_STRING);
  _emit_func_preamble(emit);
  _emit_mov(emit, cc->arg_r[1], cc->arg_r[0]);
  _emit_instr(emit, "lea %s, " PRINT_STRING_FMT_STR "[%s]", cc->arg_r[0],
              cc->rip);
  _emit_syscall(emit, "printf");
  _emit_func_ret(emit);
}

// Safely prompts the user for an integer input. If a string is entered, it
// Inteprets the first character as an ASCII integer
void _emit_input_int(Emitter *emit) {
  const CallingConvention *cc = emit->cc;
  _emit_label(emit, INPUT_INTEGER);
  _emit_func_preamble(emit);

  // Allocate 128 bytes on stack for input buffer
  _emit_sub(emit, cc->rsp, "128");

  // lea rax, [rbp-112] - buffer for input string
  _emit_lea_fmt(emit, cc->ret_r, "[%s-112]", cc->rbp);
  _emit_mov(emit, cc->arg_r[1], cc->ret_r); // rsi = buffer address
  _emit_lea_fmt(emit, cc->arg_r[0], INPUT_INTEGER_FMT_STR "[%s]",
                cc->rip); // rdi = format string
  _emit_syscall(emit, "scanf");

  // Check if scanf failed (returned -1)
  _emit_instr(emit, "cmp %s, -1", cc->ret_r);
  _emit_instr(emit, "jne .L2");
  _emit_mov(emit, cc->ret_r, "0"); // Return 0 on scanf failure
  _emit_instr(emit, "jmp .L7");

  _emit_label(emit, ".L2");
  // Try to convert string to long using strtol
  _emit_lea_fmt(emit, cc->scratch_r[0], "[%s-120]", cc->rbp); // endptr location
  _emit_lea_fmt(emit, cc->ret_r, "[%s-112]", cc->rbp);        // string buffer
  _emit_mov(emit, cc->arg_r[2], "10");
  _emit_mov(emit, cc->arg_r[1], cc->scratch_r[0]); // rsi = endptr
  _emit_mov(emit, cc->arg_r[0], cc->ret_r);        // rdi = string
  _emit_syscall(emit, "strtol");

  // Store result in [rbp-8]
  _emit_instr(emit, "mov QWORD PTR [%s-8], %s", cc->rbp, cc->ret_r);

  // Check if endptr equals original string (no conversion occurred)
  _emit_instr(emit, "mov %s, QWORD PTR [%s-120]", cc->scratch_r[1],
              cc->rbp);                                // endptr value
  _emit_lea_fmt(emit, cc->ret_r, "[%s-112]", cc->rbp); // original string
  _emit_instr(emit, "cmp %s, %s", cc->scratch_r[1], cc->ret_r);
  _emit_instr(emit, "jne .L4");

  // No conversion - use first character as ASCII value
  _emit_instr(emit, "movzx %s, BYTE PTR [%s-112]", cc->ret_r, cc->rbp);
  // TODO: There's currently no mechanism in place for different register
  // variants
  _emit_instr(emit, "movsx %s, al", cc->ret_r);
  _emit_instr(emit, "jmp .L7");

  _emit_label(emit, ".L4");
  // Check if converted value is within int32 range
  _emit_instr(emit, "cmp QWORD PTR [%s-8], 2147483647", cc->rbp);
  _emit_instr(emit, "jg .L5");
  _emit_instr(emit, "cmp QWORD PTR [%s-8], -2147483648", cc->rbp);
  _emit_instr(emit, "jge .L6");

  _emit_label(emit, ".L5");
  // Out of range - return 0
  _emit_instr(emit, "mov %s, 0", cc->ret_r);
  _emit_instr(emit, "jmp .L7");

  _emit_label(emit, ".L6");
  // In range - return the converted value
  _emit_instr(emit, "mov %s, QWORD PTR [%s-8]", cc->ret_r, cc->rbp);

  _emit_label(emit, ".L7");
  _emit_func_ret(emit);
}

// We are storing uninitialized integers in the data sector using
// var_name: .skip 8
// This initialized 8 bytes of memory (QWORD) which can be referenced later
// using mov QWORD PTR var_name[rip], 10
void _emit_symbols(Emitter *emit) {
  const VariableTable symbol_table = emit->table->variable_table;
  const size_t symbol_len = shlenu(symbol_table);
  for (size_t i = 0; i < symbol_len; i++) {
    const IdentifierHash sym = symbol_table[i];
    _emit_instr(emit, "%s%s: .skip 8", SYMBOL_DELIMITER, sym.key);
  }
}

// Emits a primary identifier and places the result in rax
// primary ::= number | indetifier
void _emit_primary(Emitter *emit, NodeID primary_node) {
  AST *ast = emit->ast;
  const NodeID num_or_ident = ast_get_first_child(ast, primary_node);
  // can only be number or identifier
  if (num_or_ident == NO_NODE || ast_node_is_grammar(ast, num_or_ident))
    return;
  const Token *token = ast_node_get_token(ast, num_or_ident);
  if (token->type == TOKEN_NUMBER) {
    _emit_mov(emit, emit->cc->ret_r, token->text);
  } else if (token->type == TOKEN_IDENT) {
    _emit_instr(emit, "mov %s, QWORD PTR %s%s[%s]", emit->cc->ret_r,
                SYMBOL_DELIMITER, token->text, emit->cc->rip);
  }
  // ERROR bad primary formed
}

// Emits a unary identifier and places the result in rax
// unary ::= ["+" | "-"] primary
void _emit_unary(Emitter *emit, NodeID unary_node) {
  AST *ast = emit->ast;
  // Should only be unary node, obv
  if (ast_node_is_token(ast, unary_node) ||
      ast_node_get_grammar(ast, unary_node) != GRAMMAR_TYPE_UNARY)
    return;
  const NodeID first_child = ast_get_first_child(ast, unary_node);
  if (first_child == NO_NODE)
    return;
  // If its a token (+ or -)
  if (ast_node_is_token(ast, first_child)) {
    const Token *token = ast_node_get_token(ast, first_child);
    const NodeID primary_child = ast_get_next_sibling(ast, first_child);
    if (primary_child == NO_NODE)
      return;
    if (token->type == TOKEN_MINUS) {
      _emit_primary(emit, primary_child);
      _emit_instr(emit, "neg %s", emit->cc->ret_r);
      return;
    } else if (token->type == TOKEN_PLUS) {
      // Unary + is a noop
      _emit_primary(emit, primary_child);
      return;
    }
  } else {
    // else, it must be a primary
    _emit_primary(emit, first_child);
  }
}

// Emits a term, and stores the result into rax
// term ::= unary {( "/" | "*" ) unary}
void _emit_term(Emitter *emit, NodeID term_node) {
  const CallingConvention *cc = emit->cc;
  AST *ast = emit->ast;
  NodeID child = ast_get_first_child(ast, term_node);
  if (child == NO_NODE)
    return;
  _emit_unary(emit, child);
  child = ast_get_next_sibling(ast, child);
  // Handle the {( "/" | "*" ) unary} case
  while (child != NO_NODE) {
    const NodeID unary_node = ast_get_next_sibling(ast, child);
    if (!ast_node_is_token(ast, child) || unary_node == NO_NODE)
      return;
    _emit_push(emit, cc->ret_r);
    _emit_unary(emit, unary_node);
    _emit_mov(emit, cc->scratch_r[0], cc->ret_r);
    _emit_pop(emit, cc->ret_r);
    const Token *child_token = ast_node_get_token(ast, child);
    if (child_token->type == TOKEN_MULT) {
      _emit_instr(emit, "imul %s, %s", cc->ret_r, cc->scratch_r[0]);
    } else if (child_token->type == TOKEN_DIV) {
      _emit_instr(emit, "cqo");
      _emit_instr(emit, "idiv %s", cc->scratch_r[0]); // stores result in rax
    } else {
      return;
    }
    child = ast_get_next_sibling(ast, unary_node);
  }
}

// expression := term {( "-" | "+" ) term}
// Emits an expression, and places the result in rax
void _emit_expression(Emitter *emit, NodeID expr_node) {
  const CallingConvention *cc = emit->cc;
  AST *ast = emit->ast;
  NodeID child = ast_get_first_child(ast, expr_node);
  if (child == NO_NODE)
    return;
  _emit_term(emit, child);
  child = ast_get_next_sibling(ast, child);
  // Handle the {( "/" | "*" ) unary} case
  while (child != NO_NODE) {
    const NodeID term_node = ast_get_next_sibling(ast, child);
    if (!ast_node_is_token(ast, child) || term_node == NO_NODE)
      return;
    _emit_push(emit, cc->ret_r);
    _emit_term(emit, term_node);
    _emit_mov(emit, cc->scratch_r[0],
              cc->ret_r); // Move the unary node into rbx
    _emit_pop(emit, cc->ret_r);
    const Token *child_token = ast_node_get_token(ast, child);
    if (child_token->type == TOKEN_PLUS) {
      _emit_add(emit, cc->ret_r, cc->scratch_r[0]);
    } else if (child_token->type == TOKEN_MINUS) {
      _emit_sub(emit, cc->ret_r, cc->scratch_r[0]);
    } else {
      return;
    }
    child = ast_get_next_sibling(ast, term_node);
  }
}

// Stores the comparison result in the jump flag (after cmp). Returns the ID of
// the operation node comparison ::= expression ("==" | "!=" | ">" | ">=" | "<"
// | "<=") expression
NodeID _emit_comparison(Emitter *emit, NodeID comparison_node) {
  const CallingConvention *cc = emit->cc;
  AST *ast = emit->ast;
  NodeID left_expr = ast_get_first_child(ast, comparison_node);
  NodeID op_node = ast_get_next_sibling(ast, left_expr);
  NodeID right_expr = ast_get_next_sibling(ast, op_node);
  if (left_expr == NO_NODE || op_node == NO_NODE || right_expr == NO_NODE)
    return NO_NODE;
  _emit_expression(emit, left_expr);
  _emit_push(emit, cc->ret_r);
  _emit_expression(emit, right_expr);
  _emit_mov(emit, cc->scratch_r[0], cc->ret_r);
  _emit_pop(emit, cc->ret_r);
  _emit_instr(emit, "cmp %s, %s", cc->ret_r, cc->scratch_r[0]);
  return op_node;
}

const char *_get_jump_instruction_from_operation(const enum TOKEN token) {
  // Switch statement isn't used bc gcc hates me
  if (token == TOKEN_EQEQ)
    return "jne";
  if (token == TOKEN_NOTEQ)
    return "je";
  if (token == TOKEN_GT)
    return "jle";
  if (token == TOKEN_GTE)
    return "jl";
  if (token == TOKEN_LT)
    return "jge";
  if (token == TOKEN_LTE)
    return "jg";
  return NULL;
}

NodeID _emit_statement_block(Emitter *emit, NodeID possible_statement_node);

void _emit_statement(Emitter *emit, NodeID statement_node) {
  const CallingConvention *cc = emit->cc;
  AST *ast = emit->ast;
  NodeID first_child = ast_get_first_child(ast, statement_node);
  if (first_child == NO_NODE || !ast_node_is_token(ast, first_child))
    return;
  const Token *token = ast_node_get_token(ast, first_child);
  // PRINT (expr | string)
  if (token->type == TOKEN_PRINT) {
    NodeID expr_or_str = ast_get_next_sibling(ast, first_child);
    // must be string
    if (ast_node_is_token(ast, expr_or_str) &&
        ast_node_get_token(ast, expr_or_str)->type == TOKEN_STRING) {
      const Token *string = ast_node_get_token(ast, expr_or_str);
      LiteralInfo literal = shget(emit->table->literal_table, string->text);
      _emit_instr(emit, "lea %s, %s%" PRIu32 "[%s]", cc->arg_r[0],
                  LITERAL_DELIMITER, literal.label, cc->rip);
      _emit_instr(emit, "call %s", PRINT_STRING);
      return;
    }
    // Otherwise, check if it's an expression node
    if (ast_node_is_grammar(ast, expr_or_str) &&
        ast_node_get_grammar(ast, expr_or_str) == GRAMMAR_TYPE_EXPRESSION) {
      // Gets expr int in rax
      _emit_expression(emit, expr_or_str);
      _emit_instr(emit, "mov %s, %s", cc->arg_r[0], cc->ret_r);
      _emit_instr(emit, "call %s", PRINT_INTEGER);
      return;
    }
    // ERROR! Bad print statement
    return;
  } else if (token->type == TOKEN_LET) {
    // "LET" ident "=" expression nl
    const NodeID ident_node = ast_get_next_sibling(ast, first_child);
    const NodeID eq_node = ast_get_next_sibling(ast, ident_node);
    const NodeID expr_node = ast_get_next_sibling(ast, eq_node);
    if (ident_node == NO_NODE || eq_node == NO_NODE || expr_node == NO_NODE)
      return;
    const Token *ident_token = ast_node_get_token(ast, ident_node);
    // Get identifier name and write value
    _emit_expression(emit, expr_node);
    _emit_instr(emit, "mov QWORD PTR %s%s[%s], %s", SYMBOL_DELIMITER,
                ident_token->text, cc->rip, cc->ret_r);
    return;
  } else if (token->type == TOKEN_INPUT) {
    // "INPUT" ident nl
    const NodeID ident_node = ast_get_next_sibling(ast, first_child);
    if (ident_node == NO_NODE)
      return;
    const Token *ident_token = ast_node_get_token(ast, ident_node);
    DZ_ASSERT(ident_token->type == TOKEN_IDENT);
    _emit_instr(emit, "call %s", INPUT_INTEGER);
    _emit_instr(emit, "mov QWORD PTR %s%s[%s], %s", SYMBOL_DELIMITER,
                ident_token->text, cc->rip, cc->ret_r);
    return;
  } else if (token->type == TOKEN_LABEL) {
    // LABEL ident
    const NodeID label_ident_node = ast_get_next_sibling(ast, first_child);
    if (label_ident_node == NO_NODE)
      return;
    const Token *ident_token = ast_node_get_token(ast, label_ident_node);
    DZ_ASSERT(ident_token->type == TOKEN_IDENT);
    _emit_instr(emit, "%s%s:", LABEL_DELIMITER, ident_token->text);
    return;
  } else if (token->type == TOKEN_GOTO) {
    const NodeID label_ident_node = ast_get_next_sibling(ast, first_child);
    if (label_ident_node == NO_NODE)
      return;
    const Token *ident_token = ast_node_get_token(ast, label_ident_node);
    DZ_ASSERT(ident_token->type == TOKEN_IDENT);
    _emit_instr(emit, "jmp %s%s", LABEL_DELIMITER, ident_token->text);
    return;
  } else if (token->type == TOKEN_IF) {
    // "IF" comparison "THEN" nl {statement}* "ENDIF" nl
    const NodeID comp_node = ast_get_next_sibling(ast, first_child);
    if (comp_node == NO_NODE)
      return;
    NodeID op_node = _emit_comparison(emit, comp_node);
    const char *jmp_inst = _get_jump_instruction_from_operation(
        ast_node_get_token(ast, op_node)->type);
    const uint32_t label_number = emitter_get_label(emit);
    _emit_instr(emit, "%s %s%" PRIu32, jmp_inst, INTERNAL_LABEL_DELIMITER,
                label_number);
    // Emit THEN body
    const NodeID then_node = ast_get_next_sibling(ast, comp_node);
    DZ_ASSERT(ast_node_get_token(ast, then_node)->type == TOKEN_THEN);
    const NodeID possible_statement = ast_get_next_sibling(ast, then_node);
    const NodeID end_if_node = _emit_statement_block(emit, possible_statement);
    DZ_ASSERT(ast_node_get_token(ast, end_if_node)->type == TOKEN_ENDIF);
    UNUSED(end_if_node);
    _emit_instr(emit, "%s%" PRIu32 ":", INTERNAL_LABEL_DELIMITER, label_number);
    return;
  } else if (token->type == TOKEN_WHILE) {
    // "WHILE" comparison "REPEAT" nl {statement}* "ENDWHILE" nl
    const NodeID comp_node = ast_get_next_sibling(ast, first_child);
    if (comp_node == NO_NODE)
      return;
    const uint32_t loop_start_label = emitter_get_label(emit);
    const uint32_t loop_end_label = emitter_get_label(emit);
    _emit_instr(emit, "%s%" PRIu32 ":", INTERNAL_LABEL_DELIMITER,
                loop_start_label);
    NodeID op_node = _emit_comparison(emit, comp_node);
    const char *jmp_inst = _get_jump_instruction_from_operation(
        ast_node_get_token(ast, op_node)->type);
    _emit_instr(emit, "%s %s%" PRIu32, jmp_inst, INTERNAL_LABEL_DELIMITER,
                loop_end_label);

    const NodeID repeat_node = ast_get_next_sibling(ast, comp_node);
    DZ_ASSERT(ast_node_get_token(ast, repeat_node)->type == TOKEN_REPEAT);
    const NodeID endwhile_node =
        _emit_statement_block(emit, ast_get_next_sibling(ast, repeat_node));
    _emit_instr(emit, "jmp %s%" PRIu32, INTERNAL_LABEL_DELIMITER,
                loop_start_label);
    UNUSED(endwhile_node);
    DZ_ASSERT(ast_node_get_token(ast, endwhile_node)->type == TOKEN_ENDWHILE);
    _emit_instr(emit, "%s%" PRIu32 ":", INTERNAL_LABEL_DELIMITER,
                loop_end_label);
  }
}

// Evaluate the results from several statements. Starts with
// possible_statement_node, and continues traversing its siblings until a
// non-statement is found, or the end of the sibling list is reached.
// Returns the next possible sibling
NodeID _emit_statement_block(Emitter *emit, NodeID possible_statement_node) {
  AST *ast = emit->ast;
  while (possible_statement_node != NO_NODE &&
         ast_node_is_grammar(ast, possible_statement_node) &&
         ast_node_get_grammar(ast, possible_statement_node) ==
             GRAMMAR_TYPE_STATEMENT) {
    _emit_statement(emit, possible_statement_node);
    possible_statement_node =
        ast_get_next_sibling(ast, possible_statement_node);
  }
  return possible_statement_node;
}

void _emit_program(Emitter *emit, NodeID program_node) {
  AST *ast = emit->ast;
  NodeID child = ast_get_first_child(ast, program_node);
  while (child != NO_NODE) {
    _emit_statement(emit, child);
    child = ast_get_next_sibling(ast, child);
  }
}

void emit_x86(const PlatformInfo *plat_info, FILE *file, AST *ast,
              NameTable *table) {
  Emitter emit = emitter_init(plat_info, file, ast, table);
  batched_writer_write(&emit.writer, PREAMBLE);
  // Here's where the static vars should go
  _emit_literals(&emit);
  _emit_symbols(&emit);
  batched_writer_write(&emit.writer, MAIN_PREAMBLE);
  _emit_func_preamble(&emit);
  NodeID head = ast_head(ast);
  if (head != NO_NODE) {
    _emit_program(&emit, head);
  }
  // Here's where the generated code should go
  _emit_func_ret(&emit);
  _emit_print_int(&emit);
  _emit_print_string(&emit);
  _emit_input_int(&emit);
  if (plat_info->os == OS_LINUX) {
    batched_writer_write(&emit.writer, LINUX_POSTAMBLE);
  }
  batched_writer_close(&emit.writer);
  return;
}
