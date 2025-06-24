#include <stdio.h>

void main() {
  int row = 0;
  int len = 0;

  while (row < 20) {
    while (len < 20) {
      int row_is_even = row % 2 == 0;
      if (len % 2 == row_is_even) printf("#");
      else printf(" ");
      len += 1;
    }

    len = 0;
    row += 1;
    printf("\n");
  }
}