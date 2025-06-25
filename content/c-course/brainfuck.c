#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define arrpush(da, item)                                                                \
  {                                                                                    \
    if ((da).len >= (da).cap) {                                                    \
      (da).cap = (da).cap == 0 ? 16 : (da).cap*2;                               \
      (da).data = realloc((da).data, (da).cap*sizeof(*(da).data));          \
    }                                                                                \
                                                                              \
    (da).data[(da).len++] = (item);                                               \
  }

#define arrdef(type, name) \
    typedef struct {   \
      size_t cap, len; \
      type* data;       \
    } name##Array;     \

typedef enum {
  Left,
  Right,
  Plus,
  Minus,
  Dot,
  Comma,
  Open,
  Close,
} Command;

arrdef(Command, Command);
typedef struct {
  CommandArray code;
  size_t ip;
  char data[30000];
  size_t dp;
} VM;


VM parse(char* src, int size) {
  CommandArray c = {0};
  
  for (int i=0; i<size; ++i) {
    switch(src[i]) {
      case '>': arrpush(c, Right); break;
      case '<': arrpush(c, Left); break;
      case '+': arrpush(c, Plus); break;
      case '-': arrpush(c, Minus); break;
      case '.': arrpush(c, Dot); break;
      case ',': arrpush(c, Comma); break;
      case '[': arrpush(c, Open); break;
      case ']': arrpush(c, Close); break;
      default: break;
    }
  }

  VM vm = { 0 };
  vm.code = c;
  return vm;
}

bool done(VM* vm) {
  return vm->ip >= vm->code.len;
}

void exec(VM* vm) {
  Command cmd = vm->code.data[vm->ip];
  bool jumped = false;

  switch (cmd) {
    case Left:  vm->dp -= 1; break;
    case Right: vm->dp += 1; break;
    case Plus:  vm->data[vm->dp] += 1; break;
    case Minus: vm->data[vm->dp] -= 1; break;
    case Dot:   putchar(vm->data[vm->dp]);    break;
    case Comma: vm->data[vm->dp] = getchar(); break;
    case Open:  {
      if (vm->data[vm->dp] != 0) break;

      // printf("Loop: inizio a %ld\n", vm->ip);
      size_t curr = vm->ip+1;
      int depth = 0;
      while (curr < vm->code.len) {
        Command c = vm->code.data[curr];
        // printf("Esamino %ld\n", curr);
        if (c == Open) depth += 1;
        else if (c == Close) {
          if (depth == 0) break;
          else depth -= 1;
        }
        curr += 1;
      }

      // printf("Trovata fine a %ld\n", curr);
      vm->ip = curr+1;
      jumped = true;
      break;
    }
    case Close: {
      if (vm->data[vm->dp] == 0) break;

      int curr = vm->ip-1;
      int depth = 0;
      while (curr >= 0) {
        Command c = vm->code.data[curr];
        if (c == Close) depth += 1;
        else if (c == Open) {
          if (depth == 0) break;
          else depth -= 1;
        }
        curr -= 1;
      }

      vm->ip = curr+1;
      jumped = true;
      break;
    }
  }

  if (!jumped) {
    vm->ip += 1;
  }
}

char* read_file_to_string(char* filename) {
  FILE* f = fopen(filename, "rb");
  if (!f) {
    perror("File could not be opened");
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  char* buf = malloc(size+1);
  if (!buf) {
    perror("No memory avaible for file");
    return NULL;
  }
  fseek(f, 0, SEEK_SET);
  int read = fread(buf, 1, size, f);
  if (read != size) {
    perror("Error while reading file");
    return NULL;
  }

  fclose(f);
  buf[size+1] = 0;
  return buf;
}

int main(int argc, char** argv) {
  if (argc == 1) {
    printf("No file provided\n");
  } else if (argc > 1) {
    char* s = read_file_to_string(argv[1]);
    // printf("%s\n", s);

    VM vm = parse(s, strlen(s));
    while(!done(&vm)) {
      exec(&vm);
    }

    printf("\nExecution ended.\n");
    printf("ip = %ld\n", vm.ip);
    printf("dp = %ld\n", vm.dp);
  }

  return 0;
}