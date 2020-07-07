#include <stdio.h>
#include "arcc.h"
Vector *nodes;
Vector *tokens;
Vector *strings;
Env *global_env;

void init(){
  nodes = new_vector();
  strings = new_vector();
  global_env = init_env();
}

char *read_file(char *path){
  FILE *fp = fopen(path, "r");
  if(!fp){
    error("cannot open %s", path);
  }

  // ファイルの長さを調べる
  if (fseek(fp, 0, SEEK_END) == -1)
    error("%s: fseek: %s", path);
  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1)
    error("%s: fseek: %s", path);

  // ファイル内容を読み込む
  char *buf = malloc(size + 2);
  fread(buf, size, 1, fp);

  // ファイルが必ず"\n\0"で終わっているようにする
  if (size == 0 || buf[size - 1] != '\n')
     buf[size++] = '\n';
  buf[size] = '\0';
  fclose(fp);
  return buf;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // initialize
  init();

  // tokenize
  char *cont = read_file(argv[1]);
  tokens = tokenize(cont);
  debug_vector_token(tokens);

  // parse
  toplevel();
  debug_vector_nodes(nodes);
  
  // generate x64
  gen_top();
  printd("===== A COMPILE FINISHED =====");
  return 0;  
}
