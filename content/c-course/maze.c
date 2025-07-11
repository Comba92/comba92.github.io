#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

typedef enum {
  None = 0,
  VertLeft = 185,
  Vert = 186,
  LeftDown = 187,
  LeftUp = 188,
  RightUp = 200,
  RightDown = 201,
  HoriUp = 202,
  HoriDown = 203,
  VertRight = 204,
  Hori = 205,
  Cross = 206,
  Empty = 255,
} CellSym;

typedef struct {
  bool left, right, down, up;
  bool visited;
} CellNode;

const int DIRECTIONS[][2] = {
  {-1, 0}, // left
  {1, 0},  // right
  {0, 1},  // down
  {0, -1}  // up
};

void dfs(CellNode maze[][32], int w, int h, int x, int y) {
  CellNode* curr = &maze[y][x];

  curr->visited = true;
  bool tries[4] = {0};
  int tries_count = 0;

  while (tries_count < 4) {
    int d = rand() % 4;
    if (!tries[d]) tries_count += 1;
    tries[d] = true;

    int dx =  x + DIRECTIONS[d][0];
    int dy =  y + DIRECTIONS[d][1];

    if (dx < 0 || dx >= w || dy < 0 || dy >= h) continue;

    CellNode* next = &maze[dy][dx];
    if (next->visited) continue;

    switch (d) {
      case 0: {
        curr->left = true;
        next->right = true;
        break;
      }
      case 1: {
        curr->right = true;
        next->left = true;
        break;
      }
      case 2: {
        curr->down = true;
        next->up = true;
        break;
      }
      case 3: {
        curr->up = true;
        next->down = true;
        break;
      }
    }

    dfs(maze, w, h, dx, dy);
  }
}

/*
lrdu
0000 => empty
0001 => vert
0010 => vert
0011 => vert
0100 => hori
0101 => rightup
0110 => rightdown
0111 => vertright
1000 => hori
1001 => leftup
1010 => leftdown
1011 => vertleft
1100 => hori
1101 => horiup
1110 => horidown
1111 => cross
*/

char cell_to_char(CellNode* cell) {
  int code = (cell->left << 3) 
    | (cell->right << 2)
    | (cell->down << 1) 
    | (cell->up << 0);
      
  char c;
  switch (code) {
    case 1:
    case 2:
    case 3: c = Vert; break;
    
    case 4:
    case 8:
    case 12: c = Hori; break;

    case 5: c = RightUp; break;
    case 6: c = RightDown; break;
    case 7: c = VertRight; break;

    case 9:  c = LeftUp; break;
    case 10: c = LeftDown; break;
    case 11: c = VertLeft; break;

    case 13: c = HoriUp; break;
    case 14: c = HoriDown; break;
    case 15: c = Cross; break;
    default: c = Empty; break;
  }
  return c;
}

void print(CellNode maze[][32], int w, int h) {
  for(int i=0; i<h; ++i) {
    for(int j=0; j<w; ++j) {
      CellNode* curr = &maze[i][j];
      int code = (curr->left << 3) 
        | (curr->right << 2)
        | (curr->down << 1) 
        | (curr->up << 0);
      
      char c;
      switch (code) {
        case 1:
        case 2:
        case 3: c = Vert; break;
        
        case 4:
        case 8:
        case 12: c = Hori; break;

        case 5: c = RightUp; break;
        case 6: c = RightDown; break;
        case 7: c = VertRight; break;

        case 9:  c = LeftUp; break;
        case 10: c = LeftDown; break;
        case 11: c = VertLeft; break;

        case 13: c = HoriUp; break;
        case 14: c = HoriDown; break;
        case 15: c = Cross; break;
        default: c = Empty; break;
      }

      putchar(c);
    }

    putchar('\n');
  }
}

void better_print(CellNode maze[][32], int w, int h) {
  char framebuf[h][w * 2];
  
  for(int i=0; i<h; ++i) {
    for(int j=0; j<w; ++j) {
      CellNode* curr = &maze[i][j];
      if (!curr->down) {
        if (i != h-1) {
          CellNode* below = &maze[i+1][j];
          if (below->left) framebuf[i][j*2+0] = '_';
        } else {
          framebuf[i][j*2+0] = '_';
        }

        framebuf[i][j*2+1] = '_';
      }
      if (!curr->left) framebuf[i][j*2] = '|';
    }
  }

  putchar(' ');
  for(int j=1; j<w*2; ++j) {
    putchar('_');
  }
  putchar('\n');

  for(int i=0; i<h; ++i) {
    for(int j=0; j<w*2; ++j) {
      char c = framebuf[i][j];
      if (c == 0) c = ' ';
      putchar(c);
    }

    putchar('|');
    putchar('\n');
  }
}

void better_print_large(CellNode maze[][32], int w, int h) {
  char framebuf[h][w * 3];
  
  for(int i=0; i<h; ++i) {
    for(int j=0; j<w; ++j) {
      CellNode* curr = &maze[i][j];
      if (!curr->down) {
        if (i != h-1) {
          CellNode* below = &maze[i+1][j];
          if (below->left) framebuf[i][j*3+0] = '_';
        } else {
          framebuf[i][j*3+0] = '_';
        }

        framebuf[i][j*3+1] = '_';
      }
      if (!curr->left) framebuf[i][j*3] = '|';
    }
  }

  putchar(' ');
  for(int j=1; j<w*3; ++j) {
    putchar('_');
  }
  putchar('\n');

  for(int i=0; i<h; ++i) {
    for(int j=0; j<w*3; ++j) {
      char c = framebuf[i][j];
      if (c == 0) c = ' ';
      putchar(c);
    }

    putchar('|');
    putchar('\n');
  }
}

int main() {
  srand(time(NULL));

  CellNode maze[20][32] = {0};
  dfs(maze, 32, 20, 0, 0);
  print(maze, 32, 20);
  better_print(maze, 32, 20);
  better_print_large(maze, 32, 20);

  return 0;
}