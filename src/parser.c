#include "parser.h"

typedef enum AST_NODE_TYPE {
  AST_NODE_TYPE_LEAF,
  AST_NODE_TYPE_GRAMMAR,
} AST_NODE_TYPE;

struct ASTNode {
  const AST_NODE_TYPE node_type;
  union {
    const Token token; // For Leaf Nodes, which are always tokens from the lexer
    const GrammarNode grammar; // For intermediary Grammer tokens
  } node;
};
