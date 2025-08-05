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

void _emit_literals(FILE *file, const LiteralTable literals) {
  const size_t literal_len = shlenu(literals);
  for (size_t i = 0; i < literal_len; i++) {
    const LiteralHash lit = literals[i];
    fprintf(file, "\t%s%ld: .string \"%s\"\n", LITERAL_DELIMITER,
            lit.value.label, lit.key);
  }
}

// We are storing uninitialized integers in the data sector using
// var_name: .skip 4
// This initialized 4 bytes of memory (DWORD) which can be referenced later
// using mov DWORD PTR var_name[rip], 10
void _emit_symbols(FILE *file, const SymbolTable symbol_table) {
  const size_t symbol_len = (size_t)shlenu(symbol_table);
  for (size_t i = 0; i < symbol_len; i++) {
    const SymbolHash sym = symbol_table[i];
    fprintf(file, "\t%s%s: .skip 8\n", SYMBOL_DELIMITER, sym.key);
  }
}

// Emits a primary identifier and places the result in rax
// primary ::= number | indetifier
void _emit_primary(FILE *file, AST *ast, NodeID primary_node,
                   VariableTable *table) {
  UNUSED(table);
  const NodeID num_or_ident = ast_get_first_child(ast, primary_node);
  // can only be number or identifier
  if (num_or_ident == NO_NODE || ast_node_is_grammar(ast, num_or_ident))
    return;
  const Token *token = ast_node_get_token(ast, num_or_ident);
  if (token->type == TOKEN_NUMBER) {
    fprintf(file, "\tmov rax, %s\n", token->text);
  } else if (token->type == TOKEN_IDENT) {
    fprintf(file, "\tmov rax, QWORD PTR %s%s[rip]\n", SYMBOL_DELIMITER,
            token->text);
  }
  // ERROR bad primary formed
}

// Emits a unary identifier and places the result in rax
// unary ::= ["+" | "-"] primary
void _emit_unary(FILE *file, AST *ast, NodeID unary_node,
                 VariableTable *table) {
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
      _emit_primary(file, ast, primary_child, table);
      fprintf(file, "\tneg rax\n");
      return;
    } else if (token->type == TOKEN_PLUS) {
      // Unary + is a noop
      _emit_primary(file, ast, primary_child, table);
      return;
    }
  } else {
    // else, it must be a primary
    _emit_primary(file, ast, first_child, table);
  }
}

// Emits a term, and stores the result into rax
// term ::= unary {( "/" | "*" ) unary}
void _emit_term(FILE *file, AST *ast, NodeID term_node, VariableTable *table) {
  NodeID child = ast_get_first_child(ast, term_node);
  if (child == NO_NODE)
    return;
  _emit_unary(file, ast, child, table);
  child = ast_get_next_sibling(ast, child);
  // Handle the {( "/" | "*" ) unary} case
  while (child != NO_NODE) {
    const NodeID unary_node = ast_get_next_sibling(ast, child);
    if (!ast_node_is_token(ast, child) || unary_node == NO_NODE)
      return;
    fprintf(file, "\tpush rax\n");
    _emit_unary(file, ast, unary_node, table);
    fprintf(file, "\tmov rbx, rax\n"); // Move the unary node into rbx
    fprintf(file, "\tpop rax\n");
    const Token *child_token = ast_node_get_token(ast, child);
    if (child_token->type == TOKEN_MULT) {
      fprintf(file, "\timul rax, rbx\n");
    } else if (child_token->type == TOKEN_DIV) {
      fprintf(file, "\tcqo\n");
      fprintf(file, "\tidiv rbx\n"); // stores result in rax
    } else {
      return;
    }
    child = ast_get_next_sibling(ast, unary_node);
  }
}

// expression := term {( "-" | "+" ) term}
// Emits an expression, and places the result in rax
void _emit_expression(FILE *file, AST *ast, NodeID expr_node,
                      VariableTable *table) {
  NodeID child = ast_get_first_child(ast, expr_node);
  if (child == NO_NODE)
    return;
  _emit_term(file, ast, child, table);
  child = ast_get_next_sibling(ast, child);
  // Handle the {( "/" | "*" ) unary} case
  while (child != NO_NODE) {
    const NodeID term_node = ast_get_next_sibling(ast, child);
    if (!ast_node_is_token(ast, child) || term_node == NO_NODE)
      return;
    fprintf(file, "\tpush rax\n");
    _emit_term(file, ast, term_node, table);
    fprintf(file, "\tmov rbx, rax\n"); // Move the unary node into rbx
    fprintf(file, "\tpop rax\n");
    const Token *child_token = ast_node_get_token(ast, child);
    if (child_token->type == TOKEN_PLUS) {
      fprintf(file, "\tadd rax, rbx\n");
    } else if (child_token->type == TOKEN_MINUS) {
      fprintf(file, "\tsub rax, rbx\n");
    } else {
      return;
    }
    child = ast_get_next_sibling(ast, term_node);
  }
}

void _emit_statement(FILE *file, AST *ast, NodeID statement_node,
                     VariableTable *table) {
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
      LiteralInfo literal = shget(table->literal_table, string->text);
      fprintf(file, "\tlea rdi, %s%ld[rip]\n", LITERAL_DELIMITER,
              literal.label);
      fprintf(file, "\tcall %s\n", PRINT_STRING);
      return;
    }
    // Otherwise, check if it's an expression node
    if (ast_node_is_grammar(ast, expr_or_str) &&
        ast_node_get_grammar(ast, expr_or_str) == GRAMMAR_TYPE_EXPRESSION) {
      // Gets expr int in rax
      _emit_expression(file, ast, expr_or_str, table);
      fprintf(file, "\tmov rdi, rax\n");
      fprintf(file, "\tcall %s\n", PRINT_INTEGER);
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
    _emit_expression(file, ast, expr_node, table);
    fprintf(file, "\tmov QWORD PTR %s%s[rip], rax\n", SYMBOL_DELIMITER,
            ident_token->text);
    return;
  } else if (token->type == TOKEN_INPUT) {
    // "INPUT" ident nl
    const NodeID ident_node = ast_get_next_sibling(ast, first_child);
    if (ident_node == NO_NODE)
      return;
    const Token *ident_token = ast_node_get_token(ast, ident_node);
    DZ_ASSERT(ident_token->type == TOKEN_IDENT);
    fprintf(file, "\tcall %s\n", INPUT_INTEGER);
    fprintf(file, "\tmov QWORD PTR %s%s[rip], rax\n", SYMBOL_DELIMITER,
            ident_token->text);
    return;
  } else if (token->type == TOKEN_LABEL) {
    // LABEL ident
    const NodeID label_ident_node = ast_get_next_sibling(ast, first_child);
    if (label_ident_node == NO_NODE)
      return;
    const Token *ident_token = ast_node_get_token(ast, label_ident_node);
    DZ_ASSERT(ident_token->type == TOKEN_IDENT);
    fprintf(file, "%s%s:\n", LABEL_DELIMITER, ident_token->text);
    return;
  } else if (token->type == TOKEN_GOTO) {
    const NodeID label_ident_node = ast_get_next_sibling(ast, first_child);
    if (label_ident_node == NO_NODE)
      return;
    const Token *ident_token = ast_node_get_token(ast, label_ident_node);
    DZ_ASSERT(ident_token->type == TOKEN_IDENT);
    fprintf(file, "\tjmp %s%s\n", LABEL_DELIMITER, ident_token->text);
    return;
  }
}

void _emit_program(FILE *file, AST *ast, NodeID program_node,
                   VariableTable *table) {
  NodeID child = ast_get_first_child(ast, program_node);
  while (child != NO_NODE) {
    _emit_statement(file, ast, child, table);
    child = ast_get_next_sibling(ast, child);
  }
}

void emit_x86(FILE *file, AST *ast, VariableTable *table) {
  UNUSED(ast);
  fprintf(file, "%s", PREAMBLE);
  // Here's where the static vars should go
  _emit_literals(file, table->literal_table);
  _emit_symbols(file, table->symbol_table);
  fprintf(file, "%s", MAIN_PREAMBLE);
  fprintf(file, "%s", FUNC_PREAMBLE);
  NodeID head = ast_head(ast);
  if (head != NO_NODE) {
    _emit_program(file, ast, head, table);
  }
  // Here's where the generated code should go
  fprintf(file, "%s", FUNC_POSTAMBLE);
  fprintf(file, "%s", PRINT_INT_HELPER);
  fprintf(file, "%s", PRINT_STRING_HELPER);
  fprintf(file, "%s", INPUT_INTEGER_HELPER);
  fprintf(file, "%s", POSTAMBLE);
  return;
}
