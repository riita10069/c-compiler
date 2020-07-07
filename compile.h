#include <stdlib.h>

enum{
  TK_NUM = 256,
  TK_EQL,
  TK_NEQ,
  TK_LE,
  TK_GE,
  TK_IDENT,
  TK_RETURN,
  TK_EOF,
  TK_IF,
  TK_FOR,
  TK_WHILE,
  TK_ELSE,
  TK_AND,
  TK_OR,
  TK_INC,
  TK_DEC,
  TK_PLUS_EQ,
  TK_MINUS_EQ,
  TK_MUL_EQ,
  TK_DIV_EQ,
  TK_ELSE_IF,
  TK_BREAK,
  TK_CONTINUE,
  TK_INT,
  TK_TYPE,
  TK_REM_EQ,
  TK_LSHIFT,
  TK_RSHIFT,
  TK_LSHIFT_EQ,
  TK_RSHIFT_EQ,
  TK_SIZEOF,
  TK_AND_EQ,
  TK_OR_EQ,
  TK_XOR_EQ,
  TK_DO,
  TK_CHAR,
  TK_STRING,
  TK_SWITCH,
  TK_CASE,
  TK_DEFAULT
};

enum{
  ND_NUM = 256,
  ND_EQL,
  ND_NEQ,
  ND_LE,
  ND_GE,
  ND_IDENT,
  ND_RETURN,
  ND_EOF,
  ND_IF,
  ND_FOR,
  ND_WHILE,
  ND_ELSE,
  ND_BLOCK,
  ND_AND,
  ND_OR,
  ND_INC,
  ND_DEC,
  ND_FUNC,
  ND_DEC_FUNC,
  ND_FUNC_END,
  ND_BREAK,
  ND_CONTINUE,
  ND_INT,
  ND_DEREF,
  ND_ADR,
  ND_TYPE,
  ND_LSHIFT,
  ND_RSHIFT,
  ND_DO_WHILE,
  ND_DUMMY,
  ND_GIDENT,
  ND_STRING,
  ND_SWITCH
};

typedef struct Type{
  enum {INT, PTR, ARRAY, FUNC, CHAR} ty;
  struct Type *ptr_of;
  int array_size;
}Type;

// A variable-length array.
typedef struct {
  void **data;
  int cap;
  int len;
} Vector;

typedef struct{
  int ty;
  int val;
  char *name;
  char *input;
  char *string; // for string.
  Type *type; // for pointer.
} Token;

typedef struct Node{
  int ty;
  struct Node *lhs;
  struct Node *rhs;
  struct Node *cond;
  struct Node *then;
  struct Node *els;
  struct Node *init; // for(xxx; ; )
  struct Node *last; // for(;; xxx)
  Vector *items;
  Vector *conds; // for switch
  int val;
  char* val_string; // for string
  int arg_num;
  int cnt;  // for ***x
  char* name;
} Node;

typedef struct{
  Type *type;
  char* name;
} Var;

// methods for vector.
Vector *new_vector();
void push_back(Vector *v, void* elm);
void push_backs(Vector *v, char* str);

// A simple map
typedef struct{
  Vector* keys;
  Vector* values;
} Map;

// methods for map.
Map *new_map();
void *map_get(Map* m, char *key);
void map_put(Map* m, char* key, void *value);
Var *map_getv(Map* m, char *key);
void map_putv(Map* m, char* key, Var* value);
int map_geti(Map* m, char *key);
void map_puti(Map* m, char* key, int value);
Map *map_getm(Map* m, char *key);
void map_putm(Map* m, char* key, Map* value);
int map_indexOf(Map* m, char *key);
int map_len(Map* m);
int map_contains(Map* m, char* key);

typedef struct{
  int pos;
  Vector* items;
}Stack;

Stack *new_stack();
void stack_push(Stack *stack, void* v);
void *stack_pop(Stack *stack);
void *stack_peek(Stack *stack);

// environment
typedef struct {
  Map *map;
} Env;

typedef struct{
  Type *type;
  Map *scope;
} EnvDesc;

Env *init_env();
Map *register_env(char *name, Type *type);
EnvDesc *get_env_desc(char *name);
Map *get_env_scope(char *name);

// global variables.
extern Vector *nodes;
extern Vector *tokens;
extern Vector *strings;
extern Env *global_env;

void error(char* fmt, ...);
void out(char* code);
void outf(char* fmt, ...);
void outd(char* code);
void printd(char* fmt, ...);
int do_align(int x, int align);
void debug_vector_token(Vector *v);
void debug_vector_nodes(Vector *v);
void debug_variable_table();
char* stringfy_token(int tkn_kind);
char* stringfy_node(int node_kind);

// token.c
Vector *tokenize(char *p);

// parse.c
Node *add();
Node *mul();
Node *term();
Node *unary();
Node *ternary();
Node *equality();
Node *assign();
Node *stmt();
void func_body();
void toplevel();

// gen.c
void gen(Node *n);
void gen_top();
