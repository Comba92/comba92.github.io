#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#define arrdef(_type, _name) \
  typedef struct { \
    unsigned int cap, len; \
    _type* data; \
  } _name##Array \

#define arrpush(_arr, _val) \
  { \
    if ((_arr).cap == 0) { \
      (_arr).cap = 16; \
      (_arr).data = malloc(sizeof(_val) * (_arr).cap); \
    } else if ((_arr).len >= (_arr).cap) { \
      (_arr).cap *= 2; \
      (_arr).data = realloc((_arr).data, sizeof(_val) * (_arr).cap); \
    } \
 \
    (_arr).data[(_arr).len++] = (_val); \
  } \

#define arrfree(_arr) \
  { \
    if ((_arr).cap != 0) { \
      free(_arr.data); \
    } \
  } \

typedef enum {
  Token_EOF,
  Token_Number,
  Token_Symbol,
} TokenKind;

typedef enum {
  Sym_None,
  Sym_ParenLeft,
  Sym_ParenRight,
  Sym_Add,
  Sym_Sub,
  Sym_Mul,
  Sym_Div,
} SymbolKind;

typedef struct {
  TokenKind kind;
  int start, len;
  union {
    double num;
    SymbolKind sym;
  };
} Token;

Token token_new_num(double num, int start, int len) {
  return (Token) {
    .kind = Token_Number,
    .num = num,
    .start = start,
    .len = len,
  };
}

Token token_new_sym(SymbolKind sym, int start) {
  return (Token) {
    .kind = Token_Symbol,
    .sym = sym,
    .start = start,
    .len = 1,
  };
}

Token token_eof() {
  return (Token) {
    .kind = Token_EOF,
  };
}

arrdef(Token, Token);
arrdef(char, Char);
TokenArray tokenize(char* s, int size) {
  TokenArray tokens = { 0 };
  CharArray num_buf = { 0 };
  
  for (int i=0; i<size; ++i) {
    SymbolKind curr = Sym_None;
    switch (s[i]) {
      case '(': curr = Sym_ParenLeft; break;
      case ')': curr = Sym_ParenRight; break;
      case '+': curr = Sym_Add; break;
      case '-': curr = Sym_Sub; break;
      case '*': curr = Sym_Mul; break;
      case '/': curr = Sym_Div; break;
      case ' ':
      case '\n':
      case '\t': break;
      default: {
        if (isdigit(s[i])) {
          num_buf.len = 0;
          while (i < size && (isdigit(s[i]) || s[i] == '.'))
            arrpush(num_buf, s[i++]);

          arrpush(num_buf, 0);
          double num = atof(num_buf.data);
          Token t = token_new_num(num, i, num_buf.len);
          arrpush(tokens, t);
        } else {
          int err_start = i;
          while (i < size && !isblank(s[i])) i += 1;
          printf("Tokenizer error: unexpected token at %d, long %d\n", err_start, i-err_start);

          arrfree(num_buf);
          return (TokenArray) {0};
        }

        break;
      }
    }

    if (curr != Sym_None) {
      Token t = token_new_sym(curr, i);
      arrpush(tokens, t);
    }
  }

  arrfree(num_buf);
  return tokens;
}

typedef enum {
  Expr_Undefined,
  Expr_Number,
  Expr_Unary,
  Expr_Binary,
} ExprKind;


struct Expr;
struct ExprUnary;
struct ExprBinary;

struct Expr {
  ExprKind kind;
  union {
    double num;
    struct ExprUnary* unr;
    struct ExprBinary* bin;
  };
};

struct ExprUnary {
  struct Expr *right;
  Token operator;
};

struct ExprBinary {
  struct Expr *left, *right;
  Token operator;
};

typedef struct Expr Expr;
typedef struct ExprBinary ExprBinary;

Expr* expr_undefined() {
  Expr* res = malloc(sizeof(Expr));
  res->kind = Expr_Undefined;
  return res;
}

Expr* expr_new_number(double num) {
  Expr* res = malloc(sizeof(Expr));
  res->kind = Expr_Number;
  res->num = num;
  return res;
}

Expr* expr_new_binary(Expr* left, Token op, Expr* right) {
  Expr* res = malloc(sizeof(Expr));
  ExprBinary* bin = malloc(sizeof(ExprBinary));
  res->kind = Expr_Binary;
  bin->left = left;
  bin->right = right;
  bin->operator = op;
  res->bin = bin;
  return res;
}

void expr_print(Expr* e) {
  printf("Expr: %d\n", e->kind);
  switch (e->kind) {
    case Expr_Number: {
      printf("%f\n", e->num);
      break;
    }

    case Expr_Binary: {
      if (e->bin->left != NULL) expr_print(e->bin->left);
      printf("[Operator %d]\n", e->bin->operator.kind);
      if (e->bin->right != NULL) expr_print(e->bin->right);
      break;
    }

    default: {
      printf("You fucked up\n");
      break;
    }
  }
}

// Expr -> ( Add ) | Number
// Mult -> Expr (*|/ Expr)*
// Add -> Mult (+|- Mult)*

typedef struct {
  TokenArray tokens;
  int curr;
  char* err;
} Parser;

Token parser_peek(Parser* p) {
  if (p->curr >= p->tokens.len)
    return token_eof();

  return p->tokens.data[p->curr];
}

Token parser_consume(Parser* p) {
  Token t = parser_peek(p);
  p->curr += 1;
  return t;
}

Expr* parser_err(Parser* p, char* msg, Token t) {
  char* buf = malloc(256);
  snprintf(buf, 256, "%s at %d, long %d", msg, t.start, t.len);
  p->err = buf;
  return expr_undefined();
}

Expr* parse_expr(Parser* p);
Expr* parse_add(Parser* p);
Expr* parse_mul(Parser* p);

Expr* parse_expr(Parser* p) {
  Token t = parser_peek(p);

  switch (t.kind) {
    case Token_Number: {
      parser_consume(p);
      return expr_new_number(t.num);
    }

    case Token_Symbol: {
      switch (t.sym) {
        case Sym_ParenLeft: {
          parser_consume(p);
          Expr* expr = parse_add(p);
          if (parser_peek(p).sym != Sym_ParenRight) {
            return parser_err(p, "missing right parenthesis", t);
          }
          parser_consume(p);
          return expr;
        }

        case Sym_ParenRight: {
          return parser_err(p, "missing left parenthesis", t);
        }

        default: {
          return parser_err(p, "unexpected symbol", t);
        }
      }
    }

    case Token_EOF: {
      return expr_undefined();
    }

    default: {
      return parser_err(p, "unexpected token", t);
    }
  }
}

Expr* parse_mul(Parser* p) {
  Expr* left = parse_expr(p);
  Token t = parser_peek(p);

  while (t.sym == Sym_Mul || t.sym == Sym_Div) {
    parser_consume(p);
    Expr* right = parse_expr(p);
    left = expr_new_binary(left, t, right);
    t = parser_peek(p);
  }

  return left;
}

Expr* parse_add(Parser* p) {
  Expr* left = parse_mul(p);
  Token t = parser_peek(p);

  while (t.sym == Sym_Add || t.sym == Sym_Sub) {
    parser_consume(p);
    Expr* right = parse_mul(p);
    left = expr_new_binary(left, t, right);
    t = parser_peek(p);
  }

  return left;
}

Expr* parse(TokenArray tokens) {
  Parser p = { .tokens = tokens, .curr = 0, .err = NULL };
  Expr* res = parse_add(&p);

  arrfree(tokens);

  if (p.err != NULL) {
    printf("Parser error: %s\n", p.err);
    return expr_undefined();
  }

  return res;
}

double evaluate(Expr* e) {
  switch (e->kind) {
    case Expr_Number: {
      double res = e->num;
      free(e);
      return res;
    }

    case Expr_Binary: {
      double left = evaluate(e->bin->left);
      double right = evaluate(e->bin->right);
      free(e->bin);
      free(e);

      switch (e->bin->operator.sym) {
        case Sym_Add: return left + right;
        case Sym_Sub: return left - right;
        case Sym_Mul: return left * right;
        case Sym_Div: return left / right;
        default: {
          printf("unexpected operatr kind\n");
          return 0;
        }
      }
    }

    default: {
      free(e);
      printf("unexpected expression kind\n");
      return 0;
    }
  }
}

int main() {
  char buf[256];

  while (true) {
    printf("> ");
    fgets(buf, 256, stdin);
    TokenArray tokens = tokenize(buf, strlen(buf));
    if (tokens.len == 0) continue;

    Expr* expr = parse(tokens);
    if (expr->kind != Expr_Undefined) {
      printf("%f\n", evaluate(expr));
    }
  }

  return 0;
}