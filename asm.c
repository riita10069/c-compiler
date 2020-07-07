#include <stdio.h>
#include <string.h>
#include "arcc.h"

typedef struct{
  int n;
  char *start;
  char *then;
  char *els;
  char *last;
  char *end;
} Labeler;

typedef struct {
  Map *table;
  int size;
} VarTable;

typedef struct {
  Type *type;
  int size;
  int offset;
} VarDesc;

static int next_label = 1;
static VarTable* global_table;
static VarTable* local_table;

static int get_type_sizeof(Type *type){
  if(type->ty == CHAR){
    return 1;
  }
  
  if(type->ty == INT){
    return 4;
  }

  if(type->ty == PTR){
    return 8;
  }

  if(type->ty == ARRAY){
    return type->array_size * get_type_sizeof(type->ptr_of);
  }

  if(type->ty == FUNC){
    return 8;
  }

  printd("A Unknown type Detected when calling get_type_sizeof()");
  return 0;
}

static VarDesc *new_var_desc(Type *type, int size, int offset){
  VarDesc *v = malloc(sizeof(VarDesc));
  v->type = type;
  v->size = size;
  v->offset = offset;
  return v;
}

static VarDesc *lookup(char *name){
  VarDesc* lvar = map_get(local_table->table, name);
  if (lvar != NULL){
    return lvar;
  }

  VarDesc * gvar = map_get(global_table->table, name);
  if (gvar != NULL){
    return gvar;
  }
  
  error("gen_x64.c ERROR : name '%s' is undefined. ", name);
  return NULL;
}

// TODO!!!!!
static VarTable* create_gvar_table(Map *env){
  Map* m = new_map();
  for(int i=0; i < env->values->len; i++){
    EnvDesc* e = ((EnvDesc*)env->values->data[i]);
    map_put(m, env->keys->data[i], new_var_desc(e->type, get_type_sizeof(e->type), 0));
  }
  VarTable *tbl = malloc(sizeof(VarTable));
  tbl->table = m;
  return tbl;
}

static VarTable* create_var_table(Map *env){
  Map* m = new_map();
  int offset = 0;
  for(int i=0; i < env->values->len; i++){
    Var *v = ((Var*)env->values->data[i]);
    offset+= get_type_sizeof(v->type);
    map_put(m, v->name, new_var_desc(v->type, get_type_sizeof(v->type),  offset));
  }

  VarTable *tbl = malloc(sizeof(VarTable));
  tbl->table = m;
  tbl->size = offset;
  return tbl;
}

static char *new_label(char *sign, int cnt){
  char *s = malloc(sizeof(char)*256);
  snprintf(s, 256, ".L%s%03d", sign, cnt);
  return s;
}

static Labeler *new_labeler(){
  Labeler *l = malloc(sizeof(Labeler));
  l->n = next_label++;
  l->start = new_label("begin", l->n);
  l->then = new_label("then", l->n);
  l->els = new_label("else", l->n);
  l->last = new_label("last", l->n);
  l->end = new_label("end", l->n);
  return l;
}

// NOT COOL....
void adjust( Type* type, char *reg){
  char s[64];
  if(type->ty == PTR || type->ty == ARRAY ){
    if(type->ptr_of->ty == INT){
      sprintf(s, "add %s, %s", reg, reg);
      out(s);
      out(s);
    }else if(type->ptr_of->ty == CHAR){
      // sprintf(s, "add %s, %s", reg, reg);
      // out(s);
    }else{
      sprintf(s, "add %s, %s", reg, reg);
      out(s);
      out(s);
      out(s);
    }
  }
}

static void print_dot_comm(){
  Map *m = global_env->map;
  for(int i=0; i<m->keys->len; i++){
    char *name = m->keys->data[i];
    EnvDesc* env = m->values->data[i];
    if(env->type->ty != FUNC){
      outf("%s: .zero %d", name, get_type_sizeof(env->type));
    }
  }
}

static void print_strings(){
  // TODO 1コしか定義できない
  for(int i=0; i<strings->len; i++){
    char *s = strings->data[i];
    out(".LC0:");
    outf(".string \"%s\"", s);
  }
}

static Stack *labeler_stack;

static char *regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// todo : refactoring
static char *reg[6][9] = {
  {"","dil","","","edi","","","","rdi",},
  {"","sil","","","esi","","","","rsi",},
  {"","","","","edx","","","","rdx",},
  {"","","","","ecx","","","","rcx",},
  {"","","","","r8d","","","","r8",},
  {"","","","","r9d","","","","r9",},
};

static char *mod[9] = {"","BYTE PTR","","","DWORD PTR","","","","QWORD PTR"};
// static char *to[9] = {"","","","","eax","","","","rax"};
static char *from[9] = {"","dil","","","edi","","","","rdi"};

// TODO : 出力にコメントをつける
void gen_top(){

  printf(".intel_syntax noprefix\n");
  printf(".global main\n\n");
  printf(".data\n");
  // for global var
  print_dot_comm();
  print_strings();
  
  // init
  labeler_stack = new_stack();
  global_table = create_gvar_table(global_env->map);
  
  for(int i=0; i<nodes->len; i++){
    if(((Node*)nodes->data[i])->ty == ND_DEC_FUNC){
      // prologue
      Node *n = (Node*)nodes->data[i];

      // The scope changed.
      local_table = create_var_table(get_env_scope(n->name));

      printf("\n.text\n");
      printf("%s:\n",n->name);
      out("push rbp");
      out("mov rbp, rsp");
      outf("sub rsp, %d", do_align(local_table->size, 16));

      // Move args from register into stack.
      for(int i=0; i < n->arg_num; i++){
        char *key = local_table->table->keys->data[i];
        int size = lookup(key)->size;
        outf("mov %s [rbp-%d], %s", mod[size] , (i+1) * size, reg[i][size]);
      }
      
    }else if(((Node*)nodes->data[i])->ty == ND_FUNC_END){
      // epiogue
      outd("epiogue");
      out("leave");
      out("ret");
    }else{
      // body
      gen(nodes->data[i]);
      out("pop rax");
    }
  }
}

// 左辺の評価
int gen_lval(Node *node){
  if(node->ty != ND_IDENT && node->ty != ND_GIDENT && node->ty != ND_DEREF){
    error("ERROR : LVAL ERROR = %d", node->ty);
  }

  if(node->ty == ND_DEREF){
    // 左辺のデリファレンス値は右辺値として評価する必要がある
    gen(node->lhs);
    return 8;
  }

  if(node->ty == ND_GIDENT){
    outf("lea rax, %s[rip]", node->name);
    out("push rax");
    return lookup(node->name)->size;
  }
  
  // ND_IDENT
  int offset = lookup(node->name)->offset;
  out("mov rax, rbp");
  outf("sub rax, %d", offset); 
  out("push rax");
  return lookup(node->name)->size;
}

void gen(Node *node){  
  if(node->ty == ND_IF){
    // if statement doesn't need to push item into the stack.
    Labeler *l = new_labeler();
    
    gen(node->cond);
    out("pop rax");
    out("cmp rax, 1");

    if(node->els != NULL){
      outf("jne %s", l->els);
    }else{
      outf("jne %s", l->end);
    }
    
    // then
    gen(node->then);
    outf("jmp %s", l->end);
    
    if(node->els != NULL){
      printf("%s:\n", l->els);
      gen(node->els);
    }

    printf("%s:\n", l->end);
    return;
  }

  if(node->ty == ND_WHILE){
    Labeler *l = new_labeler();
    stack_push(labeler_stack, l);
    
    printf("%s:\n", l->start);
    printf("%s:\n", l->last); // forとの互換性のためwhileにも残しているが微妙.
    gen(node->cond);
    
    out("pop rax");
    out("cmp rax, 1");
    outf("jne %s", l->end);
    gen(node->then);

    outf("jmp %s", l->start);
    printf("%s:\n", l->end);
    stack_pop(labeler_stack);
    return;
  }

  if(node->ty == ND_DO_WHILE){
    Labeler *l = new_labeler();
    stack_push(labeler_stack, l);
    printf("%s:\n", l->start);
    gen(node->then);
    gen(node->cond);
    out("pop rax");
    out("cmp rax, 1");
    outf("je %s", l->start);
    stack_pop(labeler_stack);
    return;
  }

  if(node->ty == ND_SWITCH){
    Vector *labels = new_vector();
    stack_push(labeler_stack, new_labeler());
    
    for(int i=0; i<node->conds->len; i++){
      Labeler *l = new_labeler();
      push_back(labels, l->start);
      gen(node->cond);
      gen(node->conds->data[i]);
      out("pop rdi");
      out("pop rax");
      out("cmp rdi, rax");
      outf("je %s", l->start);
    }

    Labeler *l = new_labeler();
    outf("jmp %s", l->last);
    
    for(int i=0; i<node->items->len; i++){
      printf("%s:\n", labels->data[i]);
      gen(node->items->data[i]);
    }
    
    printf("%s:\n", l->last);
    // DEFAULT.
    if(node->then != NULL){
      gen(node->then);
    }

    l = stack_pop(labeler_stack);
    printf("%s:\n", l->end);
    return;
  }
  
  if(node->ty == ND_FOR){
    Labeler *l = new_labeler();
    stack_push(labeler_stack, l);
    
    if(node->init != NULL){
      gen(node->init);
    }
    printf("%s:\n", l->start);
    
    if(node->cond != NULL){
      gen(node->cond);
      out("pop rax");
      out("cmp rax, 1");
      outf("jne %s", l->end);
    }
    
    gen(node->then);
    
    printf("%s:\n", l->last);
    if(node->last != NULL){
      gen(node->last);
    }
    outf("jmp %s", l->start);

    printf("%s:\n", l->end);
    stack_pop(labeler_stack);
    return; 
  }

  if(node->ty == ND_BREAK){
    outf("jmp %s", ((Labeler*)stack_peek(labeler_stack))->end);
    return;
  }

  if(node->ty == ND_CONTINUE){
    outf("jmp %s", ((Labeler*)stack_peek(labeler_stack))->last);
    return;
  }
  
  if(node->ty == ND_BLOCK){
    for(int i=0; i < node->items->len; i++){
      gen(node->items->data[i]);
    }
    return;
  }
  
  if(node->ty == ND_NUM){
    printf("  push %d\n", node->val);
    return;
  }

  if(node->ty == ND_STRING){
    out("lea rax, .LC0[rip]");
    out("push rax");
    return;
  }

  if(node->ty == '~'){
    gen(node->lhs);
    out("pop rax");
    out("not rax");
    out("push rax");
    return;
  }
  
  if(node->ty == '='){
    int size = gen_lval(node->lhs);
    gen(node->rhs);
    out("pop rdi");
    out("pop rax");
    //outf("mov [%s], %s", to[size], from[size]);
    outf("mov %s [rax], %s", mod[size], from[size]);
    out("push rdi");
    return;
  }

  // &x
  if(node->ty == ND_ADR){
    gen_lval(node->lhs);
    out("pop rax");
    outf("lea rax, [rax]");
    out("push rax");
    return;
  }

  // *x
  if(node->ty == ND_DEREF){
    gen(node->lhs);
    out("pop rax");
    out("mov rax, [rax]");
    out("push rax");
    return;
  }

  // x
  if(node->ty == ND_IDENT || node->ty == ND_GIDENT){
    // 左辺値として評価したのち、右辺値に変換している
    int size = gen_lval(node);

    /* An array ignores right-value. */
    if(lookup(node->name)->type->ty == ARRAY){
      return;
    }
    
    // [HERE] A variable address at the top of stack.
    out("pop rax");

    /*** TODO: Refactring ***/
    if(size < 8){
      outf("movsx rax, %s [rax]", mod[size]);
    }else{
      out("mov rax, [rax]");
    } 
    out("push rax");

    // [HERE] A actual variable value at the top of stack.
    return;
  }

  if(node->ty == ND_INC){
    gen_lval(node->lhs);
    out("pop rax");
    out("push [rax]");
    out("mov rdi, rax");
    out("mov rax, [rax]");
    out("add rax, 1");
    out("mov [rdi], rax");
    return;
  }

  if(node->ty == ND_DEC){
    gen_lval(node->lhs);
    out("pop rax");
    out("push [rax]");
    out("mov rdi, rax");
    out("mov rax, [rax]");
    out("sub rax, 1");
    out("mov [rdi], rax");
    return;
  }
  
  if(node->ty == ND_RETURN){
    gen(node->lhs);
    out("pop rax");
    out("leave");
    out("ret");
    return;
  }

  if(node->ty == ND_FUNC){
    // if args exists
    if(node->items != NULL){
      for(int i=0; i<node->items->len; i++){
        gen(node->items->data[i]);
        outf("pop %s", regs[i]);
      }
    }
    outf("call %s", node->name);
    out("push rax");
    return;
  }

  if(node->ty == ND_OR){
    // todo : refactoring.
    gen(node->lhs);
    out("cmp rax, 1");
    char *ok = new_label("or_true", next_label);
    char *ng = new_label("or_false", next_label);
    char *end = new_label("or_end", next_label);
    next_label++;
    outf("je %s", ok);
    gen(node->rhs);
    out("cmp rax, 1");
    outf("jne %s", ng);
    printf("%s:\n", ok);
    out("push 1"); // true
    outf("jmp %s", end);
    printf("%s:\n", ng);
    out("push 0"); // false
    printf("%s:\n", end);
    return;
  }

  if(node->ty == ND_AND){
    // todo : refactoring.
    gen(node->lhs);
    out("cmp rax, 1");
    char *ng = new_label("and_false", next_label);
    char *end = new_label("and_end", next_label);
    next_label++;
    outf("jne %s", ng);
    gen(node->rhs);
    outf("jne %s", ng);
    out("push 1"); // true
    outf("jmp %s", end);
    printf("%s:\n", ng);
    out("push 0"); // false
    printf("%s:\n", end);
    return;
  }
  
  gen(node->lhs);
  gen(node->rhs);

  out("pop rdi");
  out("pop rax");
  
  switch(node->ty){
  case '+':
    if(node->lhs->ty == ND_IDENT){
      adjust(lookup(node->lhs->name)->type, "rdi");
    }
    out("add rax, rdi");
    break;
  case '-':
    if(node->lhs->ty == ND_IDENT){
      adjust(lookup(node->lhs->name)->type, "rdi");
    }
    out("sub rax, rdi");
    break;
  case '*':
    out("mul rdi");
    break;
  case '/':
    out("mov rdx, 0");
    out("div rdi");
    break;
  case '%':
    out("mov rdx, 0");
    out("div rdi");
    out("mov rax, rdx");
    break;
  case ND_EQL:
    out("cmp rax, rdi");
    out("sete al");
    out("movzb rax, al");
    break;
  case ND_NEQ:
    out("cmp rax, rdi");
    out("setne al");
    out("movzb rax, al");    
    break;
  case '<':
    out("cmp rax, rdi");
    out("setl al");
    out("movzb rax, al");    
    break;
  case ND_LE:
    out("cmp rax, rdi");
    out("setle al");
    out("movzb rax, al");
    break;
  case '&':
    // 3 & 1 
    out("and rax, rdi");
    break;
  case '|':
    // 3 | 1
    out("or rax, rdi");
    break;
  case '^':
    out("xor rax, rdi");
    break;
  case ND_LSHIFT:
    out("mov rcx, rdi");    
    out("sal rax, cl");
    break;
  case ND_RSHIFT:
    out("mov rcx, rdi");    
    out("sar rax, cl");
    break;
  }
  out("push rax");
}
