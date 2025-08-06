#include "emitter-x86.h"
#include "ast.h"
#include "dz_debug.h"
#include "symbol_table.h"
#include "token.h"
#include <stb_ds.h>
#include <stdio.h>

const char *PREAMBLE = ".intel_syntax noprefix\n"
                       ".data\n"
                       "\tprint_integer_fmt: .string \"%d\\n\"\n"
                       "\tprint_string_fmt: .string \"%s\\n\"\n";
const char *MAIN_PREAMBLE = ".text\n"
                            "\t.global main\n"
                            "main:\n";
const char *FUNC_PREAMBLE = "\tpush rbp\n"
                            "\tmov rbp, rsp\n";
const char *FUNC_POSTAMBLE = "\tleave\n"
                             "\tret\n";
const char *POSTAMBLE = ".section .note.GNU-stack,\"\",@progbits\n";
const char *PRINT_INT_HELPER = "# Given an integer in rdi, prints it\n"
                               "print_integer:\n"
                               "\tpush rbp \n"
                               "\tmov rbp, rsp\n"
                               "\tmov rsi, rdi\n"
                               "\tlea rdi, print_integer_fmt[rip]\n"
                               "\txor rax, rax\n"
                               "\tcall printf\n"
                               "\tleave\n"
                               "\tret\n";

const char *PRINT_STRING_HELPER = "# Given a string addr in rdi, prints it\n"
                                  "print_string:\n"
                                  "\tpush rbp\n"
                                  "\tmov rbp, rsp \n"
                                  "\tmov rsi, rdi\n"
                                  "\tlea rdi, print_string_fmt[rip]\n"
                                  "\txor rax, rax \n"
                                  "\tcall printf\n"
                                  "\tleave \n"
                                  "\tret\n";

// Safely prompts the user for an integer input. If a string is entered, it
// Inteprets the first character as an ASCII integer
const char *INPUT_INTEGER_HELPER = "input_integer:\n"
                                   "\tsub     rsp, 56\n"
                                   "\tmov     rdi, QWORD PTR stdout[rip]\n"
                                   "\tcall    fflush\n"
                                   "\tmov     esi, 32\n"
                                   "\tlea     rdi, [rsp+16]\n"
                                   "\tmov     rdx, QWORD PTR stdin[rip]\n"
                                   "\tcall    fgets\n"
                                   "\ttest    rax, rax\n"
                                   "\tje      .input_integer_5\n"
                                   "\tlea     rsi, [rsp+8]\n"
                                   "\tmov     edx, 10\n"
                                   "\tlea     rdi, [rsp+16]\n"
                                   "\tcall    strtol\n"
                                   "\tlea     rcx, [rsp+16]\n"
                                   "\tcmp     QWORD PTR [rsp+8], rcx\n"
                                   "\tje      .input_integer_8\n"
                                   "\tmov     edx, 2147483648\n"
                                   "\tadd     rdx, rax\n"
                                   "\tshr     rdx, 32\n"
                                   "\tjne     .input_integer_5\n"
                                   "\tadd     rsp, 56\n"
                                   "\tret\n"
                                   ".input_integer_5:\n"
                                   "\txor     eax, eax\n"
                                   "\tadd     rsp, 56\n"
                                   "\tret\n"
                                   ".input_integer_8:\n"
                                   "\tmovsx   eax, BYTE PTR [rsp+16]\n"
                                   "\tadd     rsp, 56\n"
                                   "\tret\n";

const char *PRINT_INTEGER = "print_integer";
const char *PRINT_STRING = "print_string";
const char *INPUT_INTEGER = "input_integer";

const char *LITERAL_DELIMITER = "_static_";
const char *SYMBOL_DELIMITER = "_var_";
const char *LABEL_DELIMITER = ".L";
const char *INTERNAL_LABEL_DELIMITER = ".IL";

// ----------------------
// Emitter Internals
// ----------------------

typedef struct {
  FILE *output;
  uint32_t control_flow_label; // Used to create unique labels for IF and WHILE
                               // statement
  VariableTable *table;        // Non-owning reference
  AST *ast;                    // Non-owning reference
} Emitter;

Emitter emitter_init(FILE *file, AST *ast, VariableTable *table) {
  return (Emitter){
      .ast = ast,
      .output = file,
      .table = table,
      .control_flow_label = 0,
  };
}

uint32_t emitter_get_label(Emitter *emit) { return emit->control_flow_label++; }

void _emit_literals(Emitter *emit) {
  const LiteralTable literals = emit->table->literal_table;
  const uint32_t literal_len = shlenu(literals);
  for (uint32_t i = 0; i < literal_len; i++) {
    const LiteralHash lit = literals[i];
    fprintf(emit->output, "\t%s%" PRIu32 ": .string \"%s\"\n",
            LITERAL_DELIMITER, lit.value.label, lit.key);
  }
}

// We are storing uninitialized integers in the data sector using
// var_name: .skip 4
// This initialized 4 bytes of memory (DWORD) which can be referenced later
// using mov DWORD PTR var_name[rip], 10
void _emit_symbols(Emitter *emit) {
  const SymbolTable symbol_table = emit->table->symbol_table;
  const uint32_t symbol_len = (uint32_t)shlenu(symbol_table);
  for (uint32_t i = 0; i < symbol_len; i++) {
    const SymbolHash sym = symbol_table[i];
    fprintf(emit->output, "\t%s%s: .skip 8\n", SYMBOL_DELIMITER, sym.key);
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
    fprintf(emit->output, "\tmov rax, %s\n", token->text);
  } else if (token->type == TOKEN_IDENT) {
    fprintf(emit->output, "\tmov rax, QWORD PTR %s%s[rip]\n", SYMBOL_DELIMITER,
            token->text);
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
      fprintf(emit->output, "\tneg rax\n");
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
    fprintf(emit->output, "\tpush rax\n");
    _emit_unary(emit, unary_node);
    fprintf(emit->output, "\tmov rbx, rax\n"); // Move the unary node into rbx
    fprintf(emit->output, "\tpop rax\n");
    const Token *child_token = ast_node_get_token(ast, child);
    if (child_token->type == TOKEN_MULT) {
      fprintf(emit->output, "\timul rax, rbx\n");
    } else if (child_token->type == TOKEN_DIV) {
      fprintf(emit->output, "\tcqo\n");
      fprintf(emit->output, "\tidiv rbx\n"); // stores result in rax
    } else {
      return;
    }
    child = ast_get_next_sibling(ast, unary_node);
  }
}

// expression := term {( "-" | "+" ) term}
// Emits an expression, and places the result in rax
void _emit_expression(Emitter *emit, NodeID expr_node) {
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
    fprintf(emit->output, "\tpush rax\n");
    _emit_term(emit, term_node);
    fprintf(emit->output, "\tmov rbx, rax\n"); // Move the unary node into rbx
    fprintf(emit->output, "\tpop rax\n");
    const Token *child_token = ast_node_get_token(ast, child);
    if (child_token->type == TOKEN_PLUS) {
      fprintf(emit->output, "\tadd rax, rbx\n");
    } else if (child_token->type == TOKEN_MINUS) {
      fprintf(emit->output, "\tsub rax, rbx\n");
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
  AST *ast = emit->ast;
  NodeID left_expr = ast_get_first_child(ast, comparison_node);
  NodeID op_node = ast_get_next_sibling(ast, left_expr);
  NodeID right_expr = ast_get_next_sibling(ast, op_node);
  if (left_expr == NO_NODE || op_node == NO_NODE || right_expr == NO_NODE)
    return NO_NODE;
  _emit_expression(emit, left_expr);
  fprintf(emit->output, "\tpush rax\n");
  _emit_expression(emit, right_expr);
  fprintf(emit->output, "\tmov rbx, rax\n");
  fprintf(emit->output, "\tpop rax\n");
  fprintf(emit->output, "\tcmp rax, rbx\n");
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
      fprintf(emit->output, "\tlea rdi, %s%" PRIu32 "[rip]\n",
              LITERAL_DELIMITER, literal.label);
      fprintf(emit->output, "\tcall %s\n", PRINT_STRING);
      return;
    }
    // Otherwise, check if it's an expression node
    if (ast_node_is_grammar(ast, expr_or_str) &&
        ast_node_get_grammar(ast, expr_or_str) == GRAMMAR_TYPE_EXPRESSION) {
      // Gets expr int in rax
      _emit_expression(emit, expr_or_str);
      fprintf(emit->output, "\tmov rdi, rax\n");
      fprintf(emit->output, "\tcall %s\n", PRINT_INTEGER);
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
    fprintf(emit->output, "\tmov QWORD PTR %s%s[rip], rax\n", SYMBOL_DELIMITER,
            ident_token->text);
    return;
  } else if (token->type == TOKEN_INPUT) {
    // "INPUT" ident nl
    const NodeID ident_node = ast_get_next_sibling(ast, first_child);
    if (ident_node == NO_NODE)
      return;
    const Token *ident_token = ast_node_get_token(ast, ident_node);
    DZ_ASSERT(ident_token->type == TOKEN_IDENT);
    fprintf(emit->output, "\tcall %s\n", INPUT_INTEGER);
    fprintf(emit->output, "\tmov QWORD PTR %s%s[rip], rax\n", SYMBOL_DELIMITER,
            ident_token->text);
    return;
  } else if (token->type == TOKEN_LABEL) {
    // LABEL ident
    const NodeID label_ident_node = ast_get_next_sibling(ast, first_child);
    if (label_ident_node == NO_NODE)
      return;
    const Token *ident_token = ast_node_get_token(ast, label_ident_node);
    DZ_ASSERT(ident_token->type == TOKEN_IDENT);
    fprintf(emit->output, "%s%s:\n", LABEL_DELIMITER, ident_token->text);
    return;
  } else if (token->type == TOKEN_GOTO) {
    const NodeID label_ident_node = ast_get_next_sibling(ast, first_child);
    if (label_ident_node == NO_NODE)
      return;
    const Token *ident_token = ast_node_get_token(ast, label_ident_node);
    DZ_ASSERT(ident_token->type == TOKEN_IDENT);
    fprintf(emit->output, "\tjmp %s%s\n", LABEL_DELIMITER, ident_token->text);
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
    fprintf(emit->output, "\t%s %s%" PRIu32 "\n", jmp_inst,
            INTERNAL_LABEL_DELIMITER, label_number);
    // Emit THEN body
    const NodeID then_node = ast_get_next_sibling(ast, comp_node);
    DZ_ASSERT(ast_node_get_token(ast, then_node)->type == TOKEN_THEN);
    const NodeID possible_statement = ast_get_next_sibling(ast, then_node);
    const NodeID end_if_node = _emit_statement_block(emit, possible_statement);
    DZ_ASSERT(ast_node_get_token(ast, end_if_node)->type == TOKEN_ENDIF);
    UNUSED(end_if_node);
    fprintf(emit->output, "%s%" PRIu32 ":", INTERNAL_LABEL_DELIMITER,
            label_number);
    return;
  } else if (token->type == TOKEN_WHILE) {
    // "WHILE" comparison "REPEAT" nl {statement}* "ENDWHILE" nl
    const NodeID comp_node = ast_get_next_sibling(ast, first_child);
    if (comp_node == NO_NODE)
      return;
    const uint32_t loop_start_label = emitter_get_label(emit);
    const uint32_t loop_end_label = emitter_get_label(emit);
    fprintf(emit->output, "%s%" PRIu32 ":\n", INTERNAL_LABEL_DELIMITER,
            loop_start_label);
    NodeID op_node = _emit_comparison(emit, comp_node);
    const char *jmp_inst = _get_jump_instruction_from_operation(
        ast_node_get_token(ast, op_node)->type);
    fprintf(emit->output, "\t%s %s%" PRIu32 "\n", jmp_inst,
            INTERNAL_LABEL_DELIMITER, loop_end_label);

    const NodeID repeat_node = ast_get_next_sibling(ast, comp_node);
    DZ_ASSERT(ast_node_get_token(ast, repeat_node)->type == TOKEN_REPEAT);
    const NodeID endwhile_node =
        _emit_statement_block(emit, ast_get_next_sibling(ast, repeat_node));
    fprintf(emit->output, "\tjmp %s%" PRIu32 "\n", INTERNAL_LABEL_DELIMITER,
            loop_start_label);
    UNUSED(endwhile_node);
    DZ_ASSERT(ast_node_get_token(ast, endwhile_node)->type == TOKEN_ENDWHILE);
    fprintf(emit->output, "%s%" PRIu32 ":\n", INTERNAL_LABEL_DELIMITER,
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

void emit_x86(FILE *file, AST *ast, VariableTable *table) {
  Emitter emit = emitter_init(file, ast, table);
  fprintf(emit.output, "%s", PREAMBLE);
  // Here's where the static vars should go
  _emit_literals(&emit);
  _emit_symbols(&emit);
  fprintf(emit.output, "%s", MAIN_PREAMBLE);
  fprintf(emit.output, "%s", FUNC_PREAMBLE);
  NodeID head = ast_head(ast);
  if (head != NO_NODE) {
    _emit_program(&emit, head);
  }
  // Here's where the generated code should go
  fprintf(emit.output, "%s", FUNC_POSTAMBLE);
  fprintf(emit.output, "%s", PRINT_INT_HELPER);
  fprintf(emit.output, "%s", PRINT_STRING_HELPER);
  fprintf(emit.output, "%s", INPUT_INTEGER_HELPER);
  fprintf(emit.output, "%s", POSTAMBLE);
  return;
}
