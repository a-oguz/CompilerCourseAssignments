

#include <stdio.h>

void print_int(int x) {
  printf("%d", x);
}

int print_int1(int x, int y) {
  printf("%d,%d\n", x, y);
	return x+y;
}
void print_string(const char *s) {
  printf("%s", s);
}

int read_int() {
  int i;
  scanf("%d", &i);
  return i;
}

