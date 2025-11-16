#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

char* file_read_to_string(char* path) {
  FILE* f = fopen(path, "rb");
  if (f == NULL) return NULL;
  
  if (fseek(f, 0, SEEK_END) != 0) return NULL;
  
  long size = ftell(f);
  if (size < 0) return NULL;

  char* buf = malloc(size);
  if (buf == NULL) return NULL;

  int read = fread(buf, 1, size, f);
  if (read != size) return NULL;
  else return buf;
}

void stdin_read_line(char* buf, int len) {
  int c;
  int count = 0;
  
  // we keep space for newline and zero terminator, so iter until len-2
  while(count < len-2) {
    c = getchar();
    if (c == '\n' || c == EOF) break;
    buf[count++] = c;
  }

  buf[count++] = '\n';
  buf[count] = '\0';

  // discard rest of stdin
  while(c != '\n') c = getchar();
}

#define VEC_PUSH(_vec, _val) \
{ \
  if ((_vec).len >= (_vec).cap) { \
    (_vec).cap = (_vec).cap == 0 ? 16 : (_vec).cap * 2; \
    (_vec).data = realloc((_vec).data, (_vec).cap * sizeof(_val)); \
  } \
  (_vec).data[(_vec).len++] = (_val); \
}
#define VEC_POP(_vec) (_vec).data[(_vec).len--]
#define VEC_FREE(_vec) free((_vec).data)

#define VEC_DEF(_type) \
typedef struct { \
 _type* data; \
 size_t len, cap; \
} _type##Vec \

typedef enum {
  ParenLeft = '(',
  ParenRight = ')',
  Number = 'n',
  Identifier = 'i',
  Add = '+',
  Mul = '*',
  Sub = '-',
  Div = '/',
  Rem = '%',
  Exp = '^',
  Var = 'v',
  Assign = '=',
} TokenType;

typedef struct {
  TokenType type;
  int start;
  int len;
  double val;
} Token;

VEC_DEF(Token);

Token token_sym(char c, int start) {
  return (Token) { c, start, 1, 0 };
}

int token_is_not_op(Token* t) {
  return t->type == Var || t->type == Assign || t->type == Number || t->type == Identifier;
}

void token_dbg(TokenVec tokens, int idx) {
  Token* t = &tokens.data[idx];
  printf("[TOKEN %d] kind = %c, col = %d, len = %d, val = %lf\n", idx, t->type, t->start, t->len, t->val);
}

TokenVec tokenize(char* str) {
  TokenVec tokens = {0};
  int column = 0;
  int line = 0;

  while (1) {
    // skip whitespace
    while (isspace(*str)) {
      str++;
      column++;
    }

    char c = *str;
    Token t;

    switch(c) {
      case '(':
      case ')':
      case '+':
      case '*':
      case '-':
      case '/':
      case '%':
      case '^':
      case '=':
        t = token_sym(c, column);
        break;

      case 'v':
        if (strncmp(str, "var", 3) == 0) {
          t = (Token) {Var, column, 3, 0};
        } else {
          fprintf(stderr, "[LEX ERR] invalid keyword token at %d\n", column);
          return (TokenVec) {0};
        }
        break;

      case '\n':
        column = 0;
        line++;
        break;

      case '\0':
        return tokens;

      default: {
        if (isdigit(c)) {
          int len = 1;
          while (str[len] != '\0' && isdigit(str[len])) len++;
          if (str[len] == '.') {
            len++;
            while (str[len] != '\0' && isdigit(str[len])) len++;
          }

          // silly... might be better to memcpy to a buf
          char* num = strndup(str, len);
          double val = atof(num);
          free(num);
          
          t = (Token) {Number, column, len, val};
        } else if (isalpha(c)) {
          int len = 1;
          while (str[len] != '\0' && (isalnum(str[len]) || str[len] == '_')) len++;
          t = (Token) {Identifier, column, len, 0};
        } else {
          // handle error
          fprintf(stderr, "[LEX ERR] Invalid token (%c) at col = %d\n", c, column);
          return (TokenVec) {0};
        }
      } break;
    }

    VEC_PUSH(tokens, t);
    str += t.len;
    column += t.len;
  }

  return tokens;
}

typedef enum {
  Literal,
  Variable,
  VarAssign,
  Unary,
  Binary,
} ExprType;

typedef struct {
  TokenType op;
  int expr;
} ExprUnary;

typedef struct {
  int lhs_idx;
  TokenType op;
  int rhs_idx;
} ExprBinary;

typedef struct {
  Token* token;
  int rhs_idx;
} ExprAssign;

typedef struct {
  ExprType type;
  union {
    double val;
    Token* var_tok;
    ExprAssign var;
    ExprUnary un;
    ExprBinary bin;
  };
} Expr;

VEC_DEF(Expr);

Expr literal(double val) {
  return (Expr) { Literal, .val = val };
}

Expr unary(Token* op, int rhs) {
  ExprUnary un = (ExprUnary) {op->type, rhs};
  return (Expr) { Unary, .un = un };
}

Expr binary(int lhs, Token* op, int rhs) {
  ExprBinary bin = (ExprBinary) {lhs, op->type, rhs};
  return (Expr) { Binary, .bin = bin };
}

Expr var_assign(Token* t, int rhs) {
  ExprAssign var = (ExprAssign) {t, rhs};
  return (Expr) { VarAssign, .var = var };
}

Expr variable(Token* t) {
  return (Expr) { Variable, .var_tok = t };
}

void expr_dbg(ExprVec ast, int idx) {
  Expr* e = &ast.data[idx];
  printf("[EXPR %d] type = %d ", idx, e->type);

  switch(e->type) {
    case Literal: printf("val = %lf\n", e->val); break;
    case Unary:
      printf("op = %c, rhs = %d\n", e->un.op, e->un.expr);
      break;
    case Binary: 
      printf("lhs = %d, op = %c, rhs = %d\n", e->bin.lhs_idx, e->bin.op, e->bin.rhs_idx);
      break;
    default: break; 
  }
}

typedef enum {
  NoErr = 0,
  BadToken,
  UnclosedParen,
  ExpectOperator,
  BadLeftExpr,
  ExpectAssign,
  ExpectIdentifier,
} ParseErr;

typedef struct  {
  char* src;
  TokenVec tokens;
  int curr_token;
  ExprVec ast;
  ParseErr err;
} Parser;

void parse_log_err(Parser* p, ParseErr err) {
  p->err = err;
  
  fprintf(stderr, "[PARSE ERR] ");
  switch (p->err) {
    case BadToken:
      break;
    
    case UnclosedParen:
      fprintf(stderr, "unclosed parenthesis");  
      break;

    case ExpectOperator:
      fprintf(stderr, "two consecutive expressions; expected operator");
      break;
      
    case BadLeftExpr:
      fprintf(stderr, "invalid lhs expression");
      break;

    case ExpectAssign:
      fprintf(stderr, "expected '=' token");
      break;

    case ExpectIdentifier:
      fprintf(stderr, "expected identifier token after 'var' keyword");
      break;
      
    default: break;
  }
  
  int token_id = p->curr_token-1;
  Token* t = &p->tokens.data[token_id];
  int column = t->start;
  fprintf(stderr, " at token %d (type = %c), column %d\n", token_id, t->type, column);
}

int parser_push(Parser* p, Expr e) {
  VEC_PUSH(p->ast, e);
  return p->ast.len-1;
}

Token* parser_eat(Parser* p) {
  return &p->tokens.data[p->curr_token++];
}

Token* parser_peek(Parser* p) {
  return &p->tokens.data[p->curr_token];
}

Expr* parser_get(Parser* p, int idx) {
  return &p->ast.data[idx];
}

int parser_is_at_end(Parser* p) {
  return (size_t) p->curr_token == p->tokens.len;
}

void parser_free(Parser *p) {
  VEC_FREE(p->tokens);
  VEC_FREE(p->ast);
}

typedef struct {
  int left;
  int right;
} PrecLvl;
#define PREC(_a, _b) (PrecLvl) {(_a), (_b)}

PrecLvl prefix_lvl(Token* op) {
  switch (op->type) {
    case Sub: return PREC(0, 15);
    default: return PREC(-1,-1);
  }
}

PrecLvl infix_lvl(Token* op) {
  switch (op->type) {
    case Add:
    case Sub:
      return PREC(9,9);
    case Mul:
    case Div:
      return PREC(10,10);
    case Exp:
      return PREC(11,12);
    case Rem: 
      return PREC(14,14);

    default: return PREC(-1,-1);
  }
}

PrecLvl postfix_lvl(Token* op) {
  switch (op->type) {
    default: return PREC(-1,-1);
  }  
}

int parse_expr(Parser* p, int prec_lvl) {
  Token* t = parser_eat(p);

  // parse lhs
  int lhs;
  switch (t->type) {
    case Number:
      lhs = parser_push(p, literal(t->val));
      break;

    case Identifier:
      lhs = parser_push(p, variable(t));
      break;

    case Sub:
      PrecLvl lvl = prefix_lvl(t);
      int rhs = parse_expr(p, lvl.right);
      lhs = parser_push(p, unary(t, rhs));
      break;

    case ParenLeft:
      lhs = parse_expr(p, 0);
      if (parser_eat(p)->type != ParenRight) {
        parse_log_err(p, UnclosedParen);
        return -1;
      }
      break;

    default:
      parse_log_err(p, BadLeftExpr);
      return -1;
  }

  while (1) {
    if (parser_is_at_end(p)) break;

    Token* op = parser_peek(p);
    if (token_is_not_op(op)) {
      parse_log_err(p, ExpectOperator);
      return -1;
    }

    // postfix operator goes there

    PrecLvl lvl = infix_lvl(op);
    if (lvl.left < prec_lvl) break;
    parser_eat(p);

    int rhs = parse_expr(p, lvl.right);
    lhs = parser_push(p, binary(lhs, op, rhs));
  }

  return lhs;
}

int parse_assign(Parser* p) {
  // eat var keyword
  parser_eat(p);

  Token* name = parser_eat(p);
  if (name->type != Identifier) {
    parse_log_err(p, ExpectIdentifier);
    return -1;
  }

  if (parser_eat(p)->type != Assign) {
    parse_log_err(p, ExpectAssign);
    return -1;
  }

  int rhs = parse_expr(p, 0);
  return parser_push(p, var_assign(name, rhs));
}

Parser parse(char* str) {
  TokenVec tokens = tokenize(str);
  Parser p = {0};
  p.src = str;
  
  if (tokens.len == 0) {
    p.err = BadToken;
    return p;
  }
  
  p.tokens = tokens;

  if (parser_peek(&p)->type == Var) {
    parse_assign(&p);
  } else {
    parse_expr(&p, 0);
  }

  return p;
}

typedef struct {
  char* name;
  double val;
} VarEntry;

VEC_DEF(VarEntry);

double eval_rec(Parser* p, int root, VarEntryVec* ctx) {
  Expr* e = parser_get(p, root);

  switch (e->type) {
    case Literal: {
      return e->val;
    }

    case Variable: {
      int start = e->var_tok->start;
      int len = e->var_tok->len;

      char* name = strndup(p->src + start, len);

      int present = -1;
      for (size_t i=0; i<ctx->len; i++) {
        if (strcmp(ctx->data[i].name, name) == 0) {
          present = i;
          break;
        }
      }

      if (present == -1) {
        fprintf(stderr, "[EVAL ERR] Undefined variable at expr id %d\n", root); 
        return NAN;
      } else {
        return ctx->data[present].val;
      }
    }

    case VarAssign: {
      int start = e->var.token->start;
      int len = e->var.token->len;

      char* name = strndup(p->src + start, len);
      double val = eval_rec(p, e->var.rhs_idx, ctx);
      VarEntry e = { name, val };

      int present = -1;
      for (size_t i=0; i<ctx->len; i++) {
        if (strcmp(ctx->data[i].name, name) == 0) {
          present = i;
          break;
        }
      }

      if (present == -1) {
        VEC_PUSH(*ctx, e);
      } else {
        free(ctx->data[present].name);
        ctx->data[present] = e;
      }

      return val;
    }

    case Unary: {
      double rhs = eval_rec(p, e->un.expr, ctx);

      switch(e->un.op) {
        case Sub: return -rhs;
        default:
          fprintf(stderr, "[EVAL ERR] Invalid unary operator at expr id %d\n", root); 
          return NAN;
      }
    }
    case Binary: {
      double lhs = eval_rec(p, e->bin.lhs_idx, ctx);
      double rhs = eval_rec(p, e->bin.rhs_idx, ctx);

      switch(e->bin.op) {
        case Add: return lhs + rhs;
        case Sub: return lhs - rhs;
        case Mul: return lhs * rhs;
        case Div: return lhs / rhs;
        case Rem: return fmod(lhs, rhs);
        case Exp: return pow(lhs, rhs);
        default:
          fprintf(stderr, "[EVAL ERR] Invalid binary operator at expr id %d\n", root); 
          return NAN;
      }
    }
  }

  // unreachable
  return NAN;
}

double eval(Parser* p, VarEntryVec* ctx) {
  return eval_rec(p, p->ast.len-1, ctx);
}

int main() {
  printf("Hello!\n");

  #define BUF_SIZE 1024
  char buf[BUF_SIZE];
  VarEntryVec ctx = {0};

  while(1) {
    fputs("> ", stdout);
    stdin_read_line(buf, BUF_SIZE);

    Parser parser = parse(buf);
    if (parser.err == NoErr) {
      printf("Result: %lf\n", eval(&parser, &ctx));
      // for(int i=0; i<parser.tokens.len; i++) token_dbg(parser.tokens, i);
      // for(int i=0; i<parser.ast.len; i++) expr_dbg(parser.ast, i); 
      // for(int i=0; i<ctx.len; i++) printf("Var - id: %d, name: %s, val: %lf\n", i, ctx.data[i].name, ctx.data[i].val); 
    }

    parser_free(&parser);
  }

  return 0;
}
