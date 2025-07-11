#include <stdio.h>
#include "stc_fs.h"
#include "stc_str.h"

int main(int argc, char** argv) {
  if (argc < 3) {
    printf("Missing args\n");
    return 0;
  }
  
  char* query = argv[1];
  char* path  = argv[2];

  char* contents = file_read_to_string(path);
  if (contents == NULL) return 0;

  str query_str = str_from_cstr(query);
  str contents_str = str_from_cstr(contents);

  // StrList lines = str_split_lines(contents_str);
  // listforeach(str, line, &lines) {
  //   String lower = str_to_lower(*line);
  //   int i = str_match(String_to_str(lower), query_str);
  //   if (i != -1) {
  //     printf(str_fmt "\n", str_arg(*line));
  //   }
  //   String_free(&lower);
  // }
  // StrList_free(&lines);

  StrIter it = str_iter(contents_str);
  String lower = {0};
  while (!iter_at_end(&it)) {
    str line = iter_next_line(&it);
    str_to_lower(&lower, line);
    int i = str_match(String_to_str(lower), query_str);
    if (i != -1) {
      printf(str_fmt "\n", str_arg(line));
    }
  }
  
  String_free(&lower);
  free(contents);
}